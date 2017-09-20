
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


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

