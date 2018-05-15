#pragma once
#include <algorithm>

namespace cctt
{
    namespace util
    {
        template <class Value, int array_trim=0>
        struct Slice
        {
            using value_type = Value;

            Slice(value_type const* first, int len): first{first}, len{len} {}
            Slice(value_type const* first, value_type const* last): Slice{first, int(last - first)} {}

            template <int len>
            Slice(value_type const (&arr)[len]): Slice{arr, len - array_trim} {}

            auto size() const -> int { return len; }
            auto data() const -> value_type const* { return first; }

            auto operator [] (int i) const -> value_type const& { return first[i]; }

            auto skip(int n) const -> Slice
            {
                if (n > len) n = len;
                return {first + n, len - n};
            }

            auto take(int n) const -> Slice
            {
                if (n > len) n = len;
                if (n < 0) n = 0;
                return {first, n};
            }

            auto trim(int n) const -> Slice
            {
                if (n > len) n = len;
                return {first, len - n};
            }

            auto starts_with(Slice x) -> bool { return (take(x.size()) == x); }

            auto search(Slice x) -> int
            {
                auto it = std::search(begin(), end(), x.begin(), x.end());
                return (it == end() ? -1 : int(it - begin()));
            }

            auto begin() const { return first; }
            auto end()   const { return first + len; }

            friend auto operator == (Slice a, Slice b) -> bool
            {
                return std::equal(a.begin(), a.end(), b.begin(), b.end());
            }

            friend auto operator != (Slice a, Slice b) -> bool
            {
                return !(a == b);
            }

        private:
            value_type const* first;
            int len;
        };

        extern template struct Slice<char, 1>;

        using String_Slice = Slice<char, 1>;
    }
}

