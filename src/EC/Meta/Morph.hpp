
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_META_MORPH_HPP
#define EC_META_MORPH_HPP

#include "TypeList.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename TypeA, typename TypeB>
        struct MorphHelper
        {
        };

        template <
            template <typename...> class TypeA,
            template <typename...> class TypeB,
            typename... TypesA, typename... TypesB>
        struct MorphHelper<TypeA<TypesA...>, TypeB<TypesB...> >
        {
            using type = TypeB<TypesA...>;
        };

        template <typename TypeA, typename TypeB>
        using Morph = typename MorphHelper<TypeA, TypeB>::type;
    }
}

#endif

