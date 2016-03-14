
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

        template <typename T, template <typename...> class TTypeList, typename... Types>
        struct IndexOf<T, TTypeList<T, Types...> > :
            std::integral_constant<std::size_t, 0>
        {
        };

        template <typename T, template <typename...> class TTypeList, typename Type, typename... Types>
        struct IndexOf<T, TTypeList<Type, Types...> > :
            std::integral_constant<std::size_t, 1 +
                IndexOf<T, TTypeList<Types...> >::value
            >
        {
        };
    }
}

#endif

