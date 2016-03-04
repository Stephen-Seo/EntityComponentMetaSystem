
#ifndef EC_META_INDEX_OF_HPP
#define EC_META_INDEX_OF_HPP

#include "TypeList.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename T, typename... Types>
        struct IndexOf
        {
        };

        template <typename T, typename... Types>
        struct IndexOf<T, TypeList<T, Types...> > :
            std::integral_constant<std::size_t, 0>
        {
        };

        template <typename T, typename Type, typename... Types>
        struct IndexOf<T, TypeList<Type, Types...> > :
            std::integral_constant<std::size_t, 1 +
                IndexOf<T, TypeList<Types...> >::value
            >
        {
        };
    }
}

#endif

