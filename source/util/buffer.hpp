#pragma once
#include <memory>
#include <cstddef>

#include "const-helper.macro.hpp"

namespace cctt
{
    namespace util
    {
        // A Buffer uniquely owns a fixed-sized array, whose size is determined at runtime.
        //
        // The main design goal of the Buffer is that, unlike std::vector,
        // the Buffer does default initialization (won't zero out the memory when possible).
        template <class Value>
        struct Buffer
        {
            using value_type = Value;

            Buffer() = default;

            // uninitialized buffer
            Buffer(std::size_t size_)
                : data_{new value_type [size_]}
                , size_{size_}
            {}

            // initialized buffer
            Buffer(std::size_t size_, value_type initial_)
                : data_{new value_type [size_]{initial_}}
                , size_{size_}
            {}

            auto size() const { return size_; }

            CONST_HELPER(
                auto data() CONST { return data_.get(); }
            );

            CONST_HELPER(
                auto& operator [] (std::size_t i) CONST { return data_[i]; }
            );

            CONST_HELPER( auto begin() CONST { return &data_[0    ]; } );
            CONST_HELPER( auto   end() CONST { return &data_[size_]; } );

        private:
            std::unique_ptr<value_type []> data_{};
            std::size_t size_{};
        };
    }
}

#include "const-helper.undef.hpp"

