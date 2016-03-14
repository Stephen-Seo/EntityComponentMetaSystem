
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

        template <template <typename...> class TTypeListA, template <typename...> class TTypeListB, typename... TypesA, typename... TypesB>
        struct CombineHelper<TTypeListA<TypesA...>, TTypeListB<TypesB...> >
        {
            using type = TypeList<TypesA..., TypesB...>;
        };

        template <typename TTypeListA, typename TTypeListB>
        using Combine = typename CombineHelper<TTypeListA, TTypeListB>::type;
    }
}

#endif

