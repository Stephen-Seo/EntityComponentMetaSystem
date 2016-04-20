
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed under the Academic Free License.
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
    template <typename ComponentsList, typename TagsList>
    struct Bitset :
        public std::bitset<ComponentsList::size + TagsList::size>
    {
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;

        template <typename Component>
        constexpr auto getComponentBit() const
        {
            return (*this)[EC::Meta::IndexOf<Component, Combined>::value];
        }

        template <typename Component>
        constexpr auto getComponentBit()
        {
            return (*this)[EC::Meta::IndexOf<Component, Combined>::value];
        }

        template <typename Tag>
        constexpr auto getTagBit() const
        {
            return (*this)[EC::Meta::IndexOf<Tag, Combined>::value];
        }

        template <typename Tag>
        constexpr auto getTagBit()
        {
            return (*this)[EC::Meta::IndexOf<Tag, Combined>::value];
        }

        template <typename Contents>
        static constexpr Bitset<ComponentsList, TagsList> generateBitset()
        {
            Bitset<ComponentsList, TagsList> bitset;

            EC::Meta::forEach<Contents>([&bitset] (auto t) {
                if(EC::Meta::Contains<decltype(t), Combined>::value)
                {
                    bitset[EC::Meta::IndexOf<decltype(t), Combined>::value] = true;
                }
            });

            return bitset;
        }
    };
}

#endif

