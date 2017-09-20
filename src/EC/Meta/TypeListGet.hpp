
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_META_TYPE_LIST_GET_HPP
#define EC_META_TYPE_LIST_GET_HPP

#include <type_traits>

#include "TypeList.hpp"
#include "IndexOf.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename TTypeList, typename TTTypeList, unsigned int Index>
        struct TypeListGetHelper
        {
            using type = TTypeList;
        };

        template <
            typename TTypeList,
            template <typename...> class TTTypeList,
            unsigned int Index,
            typename Type,
            typename... Rest>
        struct TypeListGetHelper<TTypeList, TTTypeList<Type, Rest...>, Index>
        {
            using type =
                typename std::conditional<
                    Index == EC::Meta::IndexOf<Type, TTypeList>::value,
                    Type,
                    typename TypeListGetHelper<
                        TTypeList, TTTypeList<Rest...>, Index>::type
                >::type;
        };

        template <typename TTypeList, unsigned int Index>
        using TypeListGet =
            typename TypeListGetHelper<TTypeList, TTypeList, Index>::type;
    }
}

#endif

