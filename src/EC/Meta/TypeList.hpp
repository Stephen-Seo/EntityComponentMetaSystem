
#ifndef EC_META_TYPE_LIST_HPP
#define EC_META_TYPE_LIST_HPP

namespace EC
{
    namespace Meta
    {
        template <typename... Types>
        struct TypeList
        {
            static constexpr std::size_t size{sizeof...(Types)};

        };
    }
}

#endif

