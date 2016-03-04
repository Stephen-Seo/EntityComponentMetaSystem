
#ifndef EC_MANAGER_HPP
#define EC_MANAGER_HPP

namespace EC
{
    template <typename ComponentsList, typename TagsList, typename Signatures>
    struct Manager
    {
    public:
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;

    private:
        using BitsetType = EC::Bitset<ComponentsList, TagsList>;

    };
}

#endif

