
#ifndef EC_META_HPP
#define EC_META_HPP

#include <type_traits>

namespace EC
{
    namespace Meta
    {
        template <typename... Types>
        struct TypeList
        {
            static constexpr std::size_t size{sizeof...(Types)};

        };

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
        using Contains = std::integral_constant<bool, ContainsHelper<T, TTypeList>::value >;
    }
}

#endif

