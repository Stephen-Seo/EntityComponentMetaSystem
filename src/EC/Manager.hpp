
#ifndef EC_MANAGER_HPP
#define EC_MANAGER_HPP

#define EC_INIT_ENTITIES_SIZE 1024

#include <cstddef>
#include <tuple>

#include "Meta/Combine.hpp"
#include "Bitset.hpp"

namespace EC
{
    template <typename ComponentsList, typename TagsList>
    struct Manager
    {
    public:
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;
        using BitsetType = EC::Bitset<ComponentsList, TagsList>;

        Manager()
        {
            entities.resize(EC_INIT_ENTITIES_SIZE);

            for(auto entity : entities)
            {
                entity->first = false;
            }
        }

        template <typename EComponentsList>
        std::size_t addEntity()
        {
            //TODO
            BitsetType newEntity;
            return 0;
        }

    private:
        using ComponentsStorage = EC::Meta::Morph<ComponentsList, std::tuple<> >;
        using EntitiesType = std::tuple<bool, BitsetType>;

        std::vector<EntitiesType> entities;

    };
}

#endif

