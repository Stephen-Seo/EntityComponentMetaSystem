
#ifndef EC_META_FOR_EACH_HPP
#define EC_META_FOR_EACH_HPP

#include <tuple>
#include <utility>
#include "Morph.hpp"

namespace EC
{
    namespace Meta
    {
        template <typename Function, typename TTuple, std::size_t... Indices>
        constexpr void forEachHelper(Function&& function, TTuple tuple, std::index_sequence<Indices...>)
        {
            return (void)std::initializer_list<int>{(function(std::get<Indices>(tuple)), 0)...};
        }

        template <typename TTypeList, typename Function>
        constexpr void forEach(Function&& function)
        {
            using TTuple = EC::Meta::Morph<TTypeList, std::tuple<> >;
            using TTupleSize = std::tuple_size<TTuple>;
            using IndexSeq = std::make_index_sequence<TTupleSize::value>;

            return forEachHelper(std::forward<Function>(function), TTuple{}, IndexSeq{});
        }
    }
}

#endif

