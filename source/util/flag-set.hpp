#pragma once
#include <type_traits>
#include <limits>

namespace cctt
{
    namespace util
    {
        namespace flag_set_detail
        {
            // In Paragraph 7.2/9 of the C++11 Standard:
            //
            // > The value of an enumerator or an object of an unscoped enumeration
            // > type is converted to an integer by integral promotion (4.5). [...]
            // > Note that this implicit enum to int conversion is not provided
            // > for a scoped enumeration. [...]
            //
            // AFAIK user cannot define implicit cast to int for scoped enum.
            template <class T>
            static constexpr auto is_scoped_enum =
                std::is_enum<T>::value &&
                !std::is_convertible<T, int>::value &&
                true;
        }

        template <class Flag_Enum, Flag_Enum last_flag=Flag_Enum::last_flag_>
        struct Flag_Set final
        {
            static_assert(
                flag_set_detail::is_scoped_enum<Flag_Enum>,
                "You must pass an `enum struct` to Flag_Set template parameter."
            );
            using flag_enum_type = Flag_Enum;
            using flag_int_type = typename std::make_unsigned<flag_enum_type>::type;

            static constexpr auto one()  -> flag_int_type { return 1; }
            static constexpr auto zero() -> flag_int_type { return 0; }
            static constexpr auto last() -> flag_int_type { return flag_int_type(last_flag); }
            static constexpr auto bits() -> flag_int_type { return flag_int_type(std::numeric_limits<flag_int_type>::digits); }

            static_assert(
                bits() >= last(),
                "The underlying type of your Flag_Enum is too small."
            );

            constexpr Flag_Set() = default;
            constexpr Flag_Set(flag_enum_type e): flags{flag_int_type(one() << flag_int_type(e))} {}

            template <class T, class U, class... Flags_or_Sets>
            constexpr Flag_Set(T a, U b, Flags_or_Sets... rest)
                : Flag_Set{a}
            {
                enable(Flag_Set{b, rest...});
            }

            constexpr auto has_all_of(Flag_Set fs) const -> bool { return ((flags & fs.get()) == fs.get()); }
            constexpr auto has_none_of(Flag_Set fs) const -> bool { return ((flags & fs.get()) == zero()); }
            constexpr auto has_some_of(Flag_Set fs) const -> bool { return !has_none_of(fs); }
            constexpr auto operator () (Flag_Set fs) const -> bool { return has_all_of(fs); }

            constexpr auto  toggle(Flag_Set fs) -> Flag_Set& { flags ^=  fs.get(); return *this; }
            constexpr auto  enable(Flag_Set fs) -> Flag_Set& { flags |=  fs.get(); return *this; }
            constexpr auto disable(Flag_Set fs) -> Flag_Set& { flags &= ~fs.get(); return *this; }
            constexpr auto  filter(Flag_Set fs) -> Flag_Set& { flags &=  fs.get(); return *this; }
            constexpr auto operator [] (Flag_Set fs) const -> Flag_Set { return fs.filter(*this); }

            constexpr auto get() const -> flag_int_type { return flags; }
            constexpr explicit operator flag_int_type () const { return get(); }

            friend constexpr auto operator == (Flag_Set a, Flag_Set b) -> bool { return (a.get() == b.get()); }
            friend constexpr auto operator != (Flag_Set a, Flag_Set b) -> bool { return (a.get() != b.get()); }

        private:
            flag_int_type flags{};
        };
    }
}

