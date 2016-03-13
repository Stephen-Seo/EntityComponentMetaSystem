
#ifndef EC_META_MATCHING_HPP
#define EC_META_MATCHING_HPP

#include "TypeList.hpp"
#include "Contains.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename TTypeListA, typename TTypeListB>
        struct MatchingHelper
        {
        };

        template <typename TTypeListA, typename TTypeListB, typename... Matching>
        struct MatchingHelperHelper
        {
        };

        template <typename TTypeListA, typename TTypeListB, typename... Matching>
        struct MatchingHelperHelper<TTypeListA, TTypeListB, TypeList<Matching...> >
        {
            using type = TypeList<Matching...>;
        };

        template <template <typename...> class TTypeListA, typename TTypeListB, typename Type, typename... Types, typename... Matching>
        struct MatchingHelperHelper<TTypeListA<Type, Types...>, TTypeListB, TypeList<Matching...> > :
            std::conditional<
                Contains<Type, TTypeListB>::value,
                MatchingHelperHelper<TTypeListA<Types...>, TTypeListB, TypeList<Matching..., Type> >,
                MatchingHelperHelper<TTypeListA<Types...>, TTypeListB, TypeList<Matching...> >
            >::type
        {
        };

        template <template <typename...> class TTypeListA, typename TTypeListB, typename Type, typename... Types>
        struct MatchingHelper<TTypeListA<Type, Types...>, TTypeListB> :
            std::conditional<
                Contains<Type, TTypeListB>::value,
                MatchingHelperHelper<TTypeListA<Types...>, TTypeListB, TypeList<Type> >,
                MatchingHelper<TTypeListA<Types...>, TTypeListB>
            >::type
        {
        };

        template <typename TTypeListA, typename TTypeListB>
        using Matching = MatchingHelper<TTypeListA, TTypeListB>;
    }
}

#endif

