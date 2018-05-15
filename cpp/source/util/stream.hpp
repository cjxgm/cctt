#pragma once
#include <nonstd/optional.hpp>
#include <stdexcept>

namespace cctt
{
    namespace util
    {
        template <class Instance>
        struct Stream;

        template <class Instance>
        struct Stream_Iterator;

        template <class Instance>
        struct Stream
        {
            auto begin() { return Stream_Iterator<Instance>{reinterpret_cast<Instance*>(this)}; }
            auto end() { return Stream_Iterator<Instance>{}; }
        };

        template <class Instance>
        struct Stream_Iterator
        {
            using instance_type = Instance;
            using value_type = typename instance_type::value_type;
            using stream_type = Stream<instance_type>;

            Stream_Iterator() = default;
            Stream_Iterator(instance_type* stream): stream{stream} { go_next(); }

            auto operator * () -> value_type& { return *get(); }
            auto operator -> () -> value_type* { return get(); }
            auto operator ++ () -> Stream_Iterator& { go_next(); return *this; }

            auto is_end() const -> bool { return (!stream || !value); }
            auto get() -> value_type* { return (value ? &*value : nullptr); }
            void go_next() { if (stream) value = stream->next(); }

        private:
            instance_type* stream{};
            nonstd::optional<value_type> value;
        };

        template <class Instance>
        auto operator == (Stream_Iterator<Instance> const& a, Stream_Iterator<Instance> const& b) -> bool
        {
            auto e0 = a.is_end();
            auto e1 = b.is_end();
            if (!e0 && !e1) throw std::logic_error{"You are not allowed to compare 2 not-end stream iterators."};
            return (e0 && e1);
        }

        template <class Instance>
        auto operator != (Stream_Iterator<Instance> const& a, Stream_Iterator<Instance> const& b) -> bool
        {
            return !(a == b);
        }
    }
}

