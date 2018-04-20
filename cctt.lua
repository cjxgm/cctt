
local function array(items)
    if items == nil then items = {} end

    local M = {}
    M.__index = M

    local function assert_has_any_item()
        if M.empty() then
            error("Array cannot be empty.")
        end
    end

    local function assert_index_in_range(idx)
        if idx < 1 or idx > M.count() then
            error("Array index out of bound.")
        end
    end

    function M.__tostring()
        return ("(%s)"):format(M.content())
    end

    function M.set(idx, value)
        assert_index_in_range(idx)
        items[idx] = value
    end

    function M.at(idx)
        assert_index_in_range(idx)
        return items[idx]
    end

    function M.empty()
        return #items == 0
    end

    function M.count()
        return #items
    end

    function M.back()
        assert_has_any_item()
        return items[#items]
    end

    function M.split_from(idx)
        if idx > #items then return array() end

        assert_index_in_range(idx)
        local result = array(table.move(items, idx, #items, 1, {}))

        for _=idx,#items do
            table.remove(items)
        end

        return result
    end

    function M.split_delimited_from(idx)
        assert_index_in_range(idx)
        assert_index_in_range(idx+1)

        local closing_delimiter = table.remove(items)
        local split = M.split_from(idx+1)
        local open_delimiter = table.remove(items)

        return split, open_delimiter, closing_delimiter
    end

    function M.push(item)
        table.insert(items, item)
    end

    function M.pop()
        assert_has_any_item()
        table.remove(items)
    end

    function M.content()
        return M.join(" ")
    end

    function M.join(separator)
        return table.concat(items, separator)
    end

    function M.each()
        local count = M.count()
        local old_index = 0
        return function()
            if old_index >= count then
                return nil
            else
                old_index = old_index + 1
                return items[old_index]
            end
        end
    end

    return setmetatable({}, M)
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
    function M.source() return source end

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

local token do
    local open_delimiter_symbols = string_set("( [ {")
    local closing_delimiter_symbols = string_set(") ] }")
    local ambiguous_open_delimiter_symbols = string_set("<")
    local ambiguous_closing_delimiter_symbols = string_set(">")
    local disambiguating_symbols = string_set(") ] } ;")
    local delimiter_pairs = {
        ["<"] = ">",
        [">"] = "<",
        ["("] = ")",
        [")"] = "(",
        ["["] = "]",
        ["]"] = "[",
        ["{"] = "}",
        ["}"] = "{",
        ["@["] = "@]",
        ["@]"] = "@[",
    }

    token = function(span, tags_string)
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

            local tags_max_width = 32
            local tags_width = str_tags:len()

            local spaces
            if tags_width <= tags_max_width then
                spaces = (" "):rep(tags_max_width - tags_width)
            else
                spaces = "\n" .. (" "):rep(tags_max_width)
            end
            print(("%s%s %s"):format(str_tags, spaces, str_span))
        end

        function M.is_open_delimiter()
            local content = span.content()
            if tags.has("symbol") then
                if open_delimiter_symbols.has(content) then return true end
                if ambiguous_open_delimiter_symbols.has(content) then return true end
                return false
            end

            if tags.has("begin-mark") then
                return true
            end

            return false
        end

        function M.is_closing_delimiter()
            local content = span.content()
            if tags.has("symbol") then
                if closing_delimiter_symbols.has(content) then return true end
                if ambiguous_closing_delimiter_symbols.has(content) then return true end
                return false
            end

            if tags.has("end-mark") then
                return true
            end

            return false
        end

        function M.is_disambiguator()
            local content = span.content()
            if tags.has("symbol") and disambiguating_symbols.has(content) then
                return true
            end

            if tags.has("end-mark") then
                return true
            end

            return false
        end

        function M.is_ambiguous_open_delimiter()
            local content = span.content()
            return tags.has("symbol") and ambiguous_open_delimiter_symbols.has(content)
        end

        function M.is_ambiguous_closing_delimiter()
            local content = span.content()
            return tags.has("symbol") and ambiguous_closing_delimiter_symbols.has(content)
        end

        function M.opposite_delimiter()
            if M.is_open_delimiter() or M.is_closing_delimiter() then
                return delimiter_pairs[span.content()]
            else
                error(("Not a delimiter: %s"):format(M.__tostring()))
            end
        end

        return setmetatable({}, M)
    end
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
        escaping_pattern = "^" .. escaping_pattern .. ".?"
        open_delim_pattern = "^" .. open_delim_pattern
        closing_delim_pattern = "^" .. closing_delim_pattern

        if try_advance_for(open_delim_pattern) then
            local quote_first = loc_first

            while true do
                loc_first = loc_last
                try_advance_for(non_escaping_pattern)

                loc_first = loc_last
                if not try_advance_for(escaping_pattern) then
                    loc_first = loc_last
                    try_advance_for(closing_delim_pattern)
                    break
                end
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
            if try_advance_for("^[0-9]+%.[0-9]*") then
                return make_token("literal number float")
            end

            if try_advance_for("^%.[0-9]+") then
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

            if try_advance_for("^[0-9]+") then
                return make_token("literal number integer dec")
            end
        end

        if try_advance_for("^[a-zA-Z_$][a-zA-Z_$0-9]*") then
            return make_token("non-literal identifier")
        end

        do -- symbol
            if try_advance_for_one_of("-> :: ++ -- && || == != <= >= += -= *= /= &= |= ^= ...") then
                return make_token("non-literal symbol")
            end

            if try_advance_for("^[][<>(){}=+,./|;:~!?%%^&*-]") then
                return make_token("non-literal symbol")
            end
        end

        -- fallback
        if try_advance_for("^.") then
            return make_token("error unrecognized")
        else
            return nil      -- end of string
        end
    end
end

local function oneshot_iterator(x)
    return function()
        local y = x
        x = nil
        return y
    end
end

local function concat_iterator(...)
    local iters = { ... }
    local iter_index = 1
    local iter_count = select('#', ...)
    return function()
        while iter_index <= iter_count do
            local x = iters[iter_index]()
            if x == nil then
                iter_index = iter_index + 1
            else
                return x
            end
        end
        return nil
    end
end

-- token_node(tk): leaf node
-- token_node(tk, child_nodes, open_delimiter, closing_delimiter): internal node
local function token_node(node_token, child_nodes, open_delimiter, closing_delimiter)
    local M = {}
    M.__index = M

    function M.__tostring()
        local content = M.pretty_content()
        return content
    end

    function M.token() return node_token end
    function M.open_delimiter() return open_delimiter end
    function M.closing_delimiter() return closing_delimiter end
    function M.is_leaf() return child_nodes == nil end

    function M.pretty_content(start_index)
        if start_index == nil then start_index = 0 end

        if M.is_leaf() then
            local content = node_token.span().pretty_content()
            return content, start_index
        else
            local tokens = array { ("%s%d%s:"):format(open_delimiter.span().pretty_content(), start_index, closing_delimiter.span().pretty_content()) }
            local lines = array { "" }

            for child in child_nodes.each() do
                if child.is_leaf() then
                    local content = child.pretty_content(start_index)
                    tokens.push(content)
                else
                    start_index = start_index + 1
                    local open = child.open_delimiter().span().pretty_content()
                    local closing = child.closing_delimiter().span().pretty_content()
                    tokens.push(("%s%d%s"):format(open, start_index, closing))

                    local content
                    content, start_index = child.pretty_content(start_index)
                    lines.push(content)
                end
            end

            lines.set(1, tokens.join(" "))
            local content = lines.join("\n")
            return content, start_index
        end
    end

    return setmetatable({}, M)
end

local function token_tree(source)
    local pending_nodes = array()
    local open_delimiters = array()

    local marks = "@[@]"
    local begin_mark_span = source_span(marks, source_location(1, 1, 1), source_location(1, 3, 3))
    local end_mark_span = source_span(marks, source_location(1, 3, 3), source_location(1, 5, 5))
    local begin_mark = token(begin_mark_span, "begin-mark")
    local end_mark = token(end_mark_span, "end-mark")

    for tk in concat_iterator(oneshot_iterator(begin_mark), token_stream(source), oneshot_iterator(end_mark)) do
        if tk.has_tag("error") then
            print("Error context:")
            tk.pretty_print()
            error("Unrecognized symbol.")
        end

        if not tk.has_tag("ignore") then
            pending_nodes.push(token_node(tk))

            ;(function()
                if tk.is_open_delimiter() then
                    open_delimiters.push(pending_nodes.count())
                    return
                end

                if tk.is_disambiguator() then
                    while pending_nodes.at(open_delimiters.back()).token().is_ambiguous_open_delimiter() do
                        open_delimiters.pop()
                    end
                    -- no "return" intentionally
                end

                if tk.is_closing_delimiter() then
                    local open_delimiter_at = open_delimiters.back()
                    local open_delimiter = pending_nodes.at(open_delimiter_at).token()
                    local closing_delimiter = tk
                    if open_delimiter.span().content() ~= closing_delimiter.opposite_delimiter() then
                        if closing_delimiter.is_ambiguous_closing_delimiter() then
                            return
                        end
                        print("Error context:")
                        open_delimiter.pretty_print()
                        closing_delimiter.pretty_print()
                        error("Delimiters are not paired.")
                    end

                    local child_nodes = pending_nodes.split_delimited_from(open_delimiter_at)
                    local span = source_span(open_delimiter.span().source(), open_delimiter.span().first(), closing_delimiter.span().last())
                    local node_token = token(span, "token-tree")
                    local node = token_node(node_token, child_nodes, open_delimiter, closing_delimiter)

                    pending_nodes.push(node)
                    open_delimiters.pop()
                    return
                end
            end)()
        end
    end

    if pending_nodes.count() ~= 1 then
        print("Remaining nodes:")
        for node in pending_nodes.each() do
            node.token().pretty_print()
        end
        error("Unmatching parenthesis.")
    end
    print(pending_nodes.back())
end

local src = "\z
([]//hello\n\z
    // wo/*rl*/d\n\z
    /* h\n\z
el*/lo */\n\z
#define HELLO(X) X \\\n\z
    + X \\\r\n\z
    - X \n\z
1.5e3 .1 +3. a[3] = 10 \z
auto x = L\"hello\";\n\z
[[cctt::test]] std::cerr << \"hello \" << T<int> x->y << (x++ > 5);\n\z
u8R\"asd( hello)a )as\"word)asd\"\"hi\"){}\z
"
if #arg == 1 then
    local path = arg[1]
    src = assert(io.open(path, "rb")):read("*a")
end
local tt = token_tree(src)

