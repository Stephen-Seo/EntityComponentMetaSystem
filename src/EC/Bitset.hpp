
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
    // Those queries will return a false bit every time.
    template <typename ComponentsList, typename TagsList>
    struct Bitset :
        public std::bitset<ComponentsList::size + TagsList::size + 1>
    {
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;

        Bitset()
        {
            (*this)[Combined::size] = false;
        }

        // TODO find a better way to handle non-member type in const
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
            (*this)[Combined::size] = false;
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
            (*this)[Combined::size] = false;
            return (*this)[index];
        }

        template <typename Contents>
        static constexpr Bitset<ComponentsList, TagsList> generateBitset()
        {
            Bitset<ComponentsList, TagsList> bitset;

            EC::Meta::forEach<Contents>([&bitset] (auto t) {
                if(EC::Meta::Contains<decltype(t), Combined>::value)
                {
                    bitset[EC::Meta::IndexOf<decltype(t), Combined>::value] =
                        true;
                }
            });

            return bitset;
        }
    };
}

#endif

