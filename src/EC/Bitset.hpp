
#ifndef EC_BITSET_HPP
#define EC_BITSET_HPP

#include <bitset>
#include "Meta/TypeList.hpp"
#include "Meta/Combine.hpp"

namespace EC
{
    template <typename ComponentsList, typename TagsList>
    struct Bitset :
        public std::bitset<ComponentsList::size + TagsList::size>
    {
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;

        template <typename Component>
        constexpr auto getComponentBit()
        {
            return (*this)[EC::Meta::IndexOf<Component, Combined>::value];
        }

        template <typename Tag>
        constexpr auto getTagBit()
        {
            return (*this)[EC::Meta::IndexOf<Tag, Combined>::value];
        }
    };
}

#endif

