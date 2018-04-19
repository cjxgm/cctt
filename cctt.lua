
local stack do
end

local list do
end

local function string_set(initialization_string)
    local items = {}

    local function assert_item_valid(item)
        if item:match("[ \t\r\n]") ~= nil then
            error(("Items of string sets cannot contain whitespaces, but got: %q"):format(item))
        end
    end

    local M = {}
    M.__index = M

    function M.__tostring()
        return ("[%s]"):format(M.content())
    end

    function M.has(item)
        assert_item_valid(item)
        return items[item] == true
    end

    function M.add(item)
        assert_item_valid(item)
        if M.has(item) then
            return false
        else
            items[item] = true
            return true
        end
    end

    function M.remove(item)
        assert_item_valid(item)
        if M.has(item) then
            items[item] = nil
            return true
        else
            return false
        end
    end

    function M.content()
        local item_list = {}
        for item in pairs(items) do
            table.insert(item_list, item)
        end
        table.sort(item_list)
        return table.concat(item_list, " ")
    end

    for item in initialization_string:gmatch("[^ \t\r\n]+") do
        M.add(item)
    end

    return setmetatable({}, M)
end

local function source_location(line, column, byte)
    if line == nil then line = 1 end
    if column == nil then column = 1 end
    if byte == nil then byte = 1 end

    local M = {}
    M.__index = M

    function M.__tostring()
        return ("%.4d:%.4d(0x%.4x)"):format(line, column, byte-1)
    end

    function M.line() return line end
    function M.column() return column end
    function M.byte() return byte end

    return setmetatable({}, M)
end

local function source_span(source, first_loc, last_loc)
    local M = {}
    M.__index = M

    function M.__tostring()
        local range = ("[%s -> %s]"):format(first_loc, last_loc)
        local content = M.pretty_content()
        return ("%s %s"):format(range, content)
    end

    function M.first() return first_loc end
    function M.last() return last_loc end

    function M.content()
        return source:sub(first_loc.byte(), last_loc.byte()-1)
    end

    function M.pretty_content()
        local content_quoted = ("%q"):format(M.content())
            :gsub("\\\"", "\"")
            :gsub("\n", "n")
            :gsub(" ", "␣")
        local content = content_quoted:sub(2, content_quoted:len() - 1)
        return content
    end

    return setmetatable({}, M)
end

local function token(span, tags_string)
    local tags = string_set(tags_string)

    local M = {}
    M.__index = M

    function M.__tostring()
        return ("%s: %s"):format(tags, span)
    end

    function M.has_tag(tag) return tags.has(tag) end
    function M.span() return span end
    function M.tags() return tags.content() end

    function M.pretty_print()
        local str_tags = tostring(tags)
        local str_span = tostring(span)

        local tags_max_width = 30
        local tags_width = str_tags:len()

        local spaces
        if tags_width <= tags_max_width then
            spaces = (" "):rep(tags_max_width - tags_width)
        else
            spaces = "\n" .. (" "):rep(tags_max_width)
        end
        print(("%s%s %s"):format(str_tags, spaces, str_span))
    end

    return setmetatable({}, M)
end

local function token_stream(source)
    source = source:gsub("\r\n", "\n"):gsub("\r", "\n")

    local loc_first = source_location()
    local loc_last  = source_location()

    local function has_remaining()
        return loc_first.byte() <= source:len()
    end

    local function try_advance_for(pattern)
        local x = source:match(pattern, loc_first.byte())
        if x == nil then
            loc_last = loc_first
            return false
        else
            local iter = x:gmatch("[^\n]*")

            local byte_pos = loc_first.byte() + x:len()
            local column_pos = loc_first.column() + iter():len()
            local line_pos = loc_first.line()

            for line in iter do
                line_pos = line_pos + 1
                column_pos = line:len() + 1
            end

            loc_last = source_location(line_pos, column_pos, byte_pos)
            return true
        end
    end

    -- all input patterns should only match 1 single character
    local function try_advance_for_quoted(open_delim_pattern, closing_delim_pattern, escaping_pattern)
        if escaping_pattern == nil then escaping_pattern = "\\" end
        if closing_delim_pattern == nil then closing_delim_pattern = "\n" end

        local non_escaping_pattern = "^[^" .. closing_delim_pattern .. escaping_pattern .. "]*"
        escaping_pattern = "^" .. escaping_pattern
        open_delim_pattern = "^" .. open_delim_pattern
        closing_delim_pattern = "^" .. closing_delim_pattern

        if try_advance_for(open_delim_pattern) then
            local quote_first = loc_first
            while true do
                loc_first = loc_last
                try_advance_for(non_escaping_pattern)

                loc_first = loc_last
                local done = not try_advance_for(escaping_pattern)

                loc_first = loc_last
                try_advance_for(closing_delim_pattern)

                if done then break end
            end
            loc_first = quote_first
            return true
        else
            return false
        end
    end

    -- items_string must be a space delimited string. They will be escaped properly
    local function try_advance_for_one_of(items_string)
        local patterns_string = items_string:gsub("([][()*.%%+-])", "%%%1")
        for pattern in patterns_string:gmatch("[^ \t\n]+") do
            if try_advance_for("^" .. pattern) then
                return true
            end
        end
        return false
    end

    local function make_span()
        return source_span(source, loc_first, loc_last)
    end

    local function make_token(tags_string)
        return token(make_span(), tags_string)
    end

    local function skip_whitespace()
        try_advance_for("^[ \t\n]*")
        loc_first = loc_last
    end

    return function()
        loc_first = loc_last
        skip_whitespace()

        if try_advance_for("^//[^\n]*") then
            return make_token("ignore comment-line")
        end

        if try_advance_for("^/%*.-%*/") then
            return make_token("ignore comment-block")
        end

        if try_advance_for_quoted("#") then
            return make_token("ignore directive")
        end

        do -- char/string
            if try_advance_for_quoted("'", "'") then
                return make_token("literal char")
            end

            if try_advance_for_quoted("\"", "\"") then
                return make_token("literal string")
            end

            if try_advance_for("^[LUu]?R\"") or try_advance_for("^u8R\"") then
                local quote_first = loc_first

                loc_first = loc_last
                try_advance_for("^[^() \t\n\\]*")
                local delimiter = make_span().content()
                local delimiter_pattern = "^" .. delimiter:gsub("%%", "%%") .. "\""

                while has_remaining() do
                    loc_first = loc_last
                    try_advance_for("^[^)]*")

                    loc_first = loc_last
                    try_advance_for("^%)")

                    loc_first = loc_last
                    if try_advance_for(delimiter_pattern) then
                        break
                    end
                end

                loc_first = quote_first
                return make_token("literal string raw")
            end
        end

        do -- number
            if try_advance_for("^[+-]?[0-9]+%.[0-9]*") then
                return make_token("literal number float")
            end

            if try_advance_for("^[+-]?%.[0-9]+") then
                return make_token("literal number float")
            end

            if try_advance_for("^0x[0-9a-fA-F]+") then
                return make_token("literal number integer hex")
            end

            if try_advance_for("^0b[01]+") then
                return make_token("literal number integer bin")
            end

            if try_advance_for("^0o[0-7]+") then
                return make_token("literal number integer oct")
            end

            if try_advance_for("^[+-]?[0-9]+") then
                return make_token("literal number integer dec")
            end
        end

        if try_advance_for("^[a-zA-Z_$][a-zA-Z_$0-9]*") then
            return make_token("non-literal identifier")
        end

        do -- symbol
            if try_advance_for_one_of("-> :: ++ -- && || == != <= >= += -= *= /= &= |= ^=") then
                return make_token("non-literal symbol")
            end

            if try_advance_for("^[][<>(){}=+,./;:~!%^&*-]") then
                return make_token("non-literal symbol")
            end
        end

        -- fallback
        if try_advance_for("^.") then
            local tk = make_token("error unrecognized")
            print(("skipping %s"):format(tk))
            return tk
        else
            return nil      -- end of string
        end
    end
end

local src = "\z
//hello\n\z
    // wo/*rl*/d\n\z
    /* h\n\z
el*/lo */\n\z
#define HELLO(X) X \\\n\z
    + X \\\r\n\z
    - X \n\z
1.5e3 .1 +3. a[3] = 10 \z
auto x = L\"hello\";\n\z
[[cctt::test]] std::cerr << \"hello \" << x->y << (x++ > 5);\n\z
u8R\"asd( hello)a )as\"word)asd\"\"hi\"\z
"
if #arg == 1 then
    local path = arg[1]
    src = assert(io.open(path, "rb")):read("*a")
end
for tk in token_stream(src) do
    if not tk.has_tag("error") then
        tk.pretty_print()
    end
end

