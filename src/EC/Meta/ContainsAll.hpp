
#ifndef EC_META_CONTAINS_ALL_HPP
#define EC_META_CONTAINS_ALL_HPP

#include "TypeList.hpp"
#include "Contains.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename TTypeListA, typename TTypeListB>
        struct ContainsAllHelper :
            std::true_type
        {
        };

        template <template <typename...> class TTypeListA, typename Type, typename... Types, typename TTypeListB>
        struct ContainsAllHelper<TTypeListA<Type, Types...>, TTypeListB> :
            std::conditional<
                Contains<Type, TTypeListB>::value,
                ContainsAllHelper<TypeList<Types...>, TTypeListB>,
                std::false_type
            >::type
        {
        };

        template <typename TTypeListA, typename TTypeListB>
        using ContainsAll = std::integral_constant<bool, ContainsAllHelper<TTypeListA, TTypeListB>::value>;
    }
}

#endif

