
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_META_FOR_EACH_DOUBLE_TUPLE_HPP
#define EC_META_FOR_EACH_DOUBLE_TUPLE_HPP

#include <tuple>
#include <utility>
#include "Morph.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename Function, typename TupleFirst,
            typename TupleSecond, std::size_t... Indices>
        constexpr void forEachDoubleTupleHelper(
            Function&& function, TupleFirst t0, TupleSecond t1,
            std::index_sequence<Indices...>)
        {
            return (void)std::initializer_list<int>{(function(
                std::get<Indices>(t0), std::get<Indices>(t1), Indices), 0)...};
        }

        template <typename TupleFirst, typename TupleSecond, typename Function>
        constexpr void forEachDoubleTuple(
            TupleFirst&& t0, TupleSecond&& t1, Function&& function)
        {
            using TTupleSize = std::tuple_size<TupleFirst>;
            using IndexSeq = std::make_index_sequence<TTupleSize::value>;

            return forEachDoubleTupleHelper(
                std::forward<Function>(function),
                std::forward<TupleFirst>(t0),
                std::forward<TupleSecond>(t1),
                IndexSeq{});
        }
    }
}

#endif

