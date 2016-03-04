
#ifndef EC_BITSET_HPP
#define EC_BITSET_HPP

#include <bitset>
#include "Meta/TypeList.hpp"

namespace EC
{
    template <typename ComponentsList, typename TagsList>
    struct Bitset :
        public std::bitset<ComponentsList::size + TagsList::size>
    {
        template <typename Component>
        constexpr auto getComponentBit()
        {
            return (*this)[EC::Meta::IndexOf<Component, ComponentsList>::value];
        }

        template <typename Tag>
        constexpr auto getTagBit()
        {
            return (*this)[ComponentsList::size + EC::Meta::IndexOf<Tag, TagsList>::value];
        }
    };
}

#endif

