
#ifndef EC_META_COMBINE_HPP
#define EC_META_COMBINE_HPP

#include "TypeList.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename TTypeListA, typename TTypeListB>
        struct CombineHelper
        {
            using type = TypeList<>;
        };

        template <typename... TypesA, typename... TypesB>
        struct CombineHelper<TypeList<TypesA...>, TypeList<TypesB...> >
        {
            using type = TypeList<TypesA..., TypesB...>;
        };

        template <typename TTypeListA, typename TTypeListB>
        using Combine = typename CombineHelper<TTypeListA, TTypeListB>::type;
    }
}

#endif

