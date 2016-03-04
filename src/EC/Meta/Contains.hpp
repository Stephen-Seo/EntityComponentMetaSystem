
#ifndef EC_META_CONTAINS_HPP
#define EC_META_CONTAINS_HPP

#include <type_traits>
#include "TypeList.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename T, typename... Types>
        struct ContainsHelper :
            std::false_type
        {
        };

        template <typename T, typename Type, typename... Types>
        struct ContainsHelper<T, TypeList<Type, Types...> > :
            std::conditional<
                std::is_same<T, Type>::value,
                std::true_type,
                ContainsHelper<T, TypeList<Types...> >
            >::type
        {
        };

        template <typename T, typename TTypeList>
        using Contains = std::integral_constant<bool, ContainsHelper<T, TTypeList>::value>;
    }
}

#endif

