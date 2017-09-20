
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


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

        template <
            typename T,
            template <typename...> class TTypeList,
            typename Type,
            typename... Types>
        struct ContainsHelper<T, TTypeList<Type, Types...> > :
            std::conditional<
                std::is_same<T, Type>::value,
                std::true_type,
                ContainsHelper<T, TTypeList<Types...> >
            >::type
        {
        };

        template <typename T, typename TTypeList>
        using Contains = std::integral_constant<
            bool, ContainsHelper<T, TTypeList>::value>;
    }
}

#endif

