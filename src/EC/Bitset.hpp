
#ifndef EC_BITSET_HPP
#define EC_BITSET_HPP

#include <bitset>
#include "Meta/TypeList.hpp"
#include "Meta/Combine.hpp"
#include "Meta/IndexOf.hpp"

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

        template <typename Contents>
        static constexpr Bitset<ComponentsList, TagsList> generateBitset()
        {
            //TODO
            Bitset<ComponentsList, TagsList> bitset;
/*
            for(unsigned int i = 0; i < Contents::size; ++i)
            {
                if(EC::Meta::Contains<EC::Meta::TypeListGet<Contents, i>, Combined>::value)
                {
                    bitset[EC::Meta::IndexOf<EC::Meta::TypeListGet<Contents, i>, Combined>::value] = true;
                }
            }
*/

            return bitset;
        }
    };
}

#endif

