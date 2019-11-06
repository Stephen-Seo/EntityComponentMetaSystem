
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_BITSET_HPP
#define EC_BITSET_HPP

#include <bitset>
#include "Meta/TypeList.hpp"
#include "Meta/Combine.hpp"
#include "Meta/IndexOf.hpp"
#include "Meta/ForEach.hpp"
#include "Meta/Contains.hpp"

namespace EC
{
    // Note bitset size is sizes of components and tags + 1
    // This is to use the last extra bit as the result of a query
    // with a Component or Tag not known to the Bitset.
    // Those queries should return a false bit every time as long as EC::Manager
    // does not change that last bit.
    template <typename ComponentsList, typename TagsList>
    struct Bitset :
        public std::bitset<ComponentsList::size + TagsList::size + 1>
    {
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;

        Bitset()
        {
            (*this)[Combined::size] = false;
        }

        template <typename Component>
        constexpr auto getComponentBit() const
        {
            auto index = EC::Meta::IndexOf<Component, Combined>::value;
            return (*this)[index];
        }

        template <typename Component>
        constexpr auto getComponentBit()
        {
            auto index = EC::Meta::IndexOf<Component, Combined>::value;
            return (*this)[index];
        }

        template <typename Tag>
        constexpr auto getTagBit() const
        {
            auto index = EC::Meta::IndexOf<Tag, Combined>::value;
            return (*this)[index];
        }

        template <typename Tag>
        constexpr auto getTagBit()
        {
            auto index = EC::Meta::IndexOf<Tag, Combined>::value;
            return (*this)[index];
        }

        template <typename Contents>
        static constexpr Bitset<ComponentsList, TagsList> generateBitset()
        {
            Bitset<ComponentsList, TagsList> bitset;

            EC::Meta::forEach<Contents>([&bitset] (auto t) {
                if constexpr (EC::Meta::Contains<decltype(t), Combined>::value)
                {
                    bitset[EC::Meta::IndexOf<decltype(t), Combined>::value] =
                        true;
                }
            });

            return bitset;
        }

        template <typename IntegralType>
        auto getCombinedBit(const IntegralType& i) {
            static_assert(std::is_integral<IntegralType>::value,
                "Parameter must be an integral type");
            if(i >= Combined::size || i < 0) {
                return (*this)[Combined::size];
            } else {
                return (*this)[i];
            }
        }

        template <typename IntegralType>
        auto getCombinedBit(const IntegralType& i) const {
            static_assert(std::is_integral<IntegralType>::value,
                "Parameter must be an integral type");
            if(i >= Combined::size || i < 0) {
                return (*this)[Combined::size];
            } else {
                return (*this)[i];
            }
        }
    };
}

#endif

