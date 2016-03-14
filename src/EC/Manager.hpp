
#ifndef EC_MANAGER_HPP
#define EC_MANAGER_HPP

#define EC_INIT_ENTITIES_SIZE 256
#define EC_GROW_SIZE_AMOUNT 256

#include <cstddef>
#include <tuple>
#include <utility>

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

    private:
        template <typename... Types>
        struct Storage
        {
            using type = std::tuple<std::vector<Types>... >;
        };
        using ComponentsStorage = typename EC::Meta::Morph<ComponentsList, Storage<> >::type;
        // Entity: isAlive, dataIndex, ComponentsTags Info
        using EntitiesTupleType = std::tuple<bool, std::size_t, BitsetType>;
        using EntitiesType = std::vector<EntitiesTupleType>;

        EntitiesType entities;
        ComponentsStorage componentsStorage;
        std::size_t currentCapacity = 0;
        std::size_t currentSize = 0;

    public:
        Manager()
        {
            resize(EC_INIT_ENTITIES_SIZE);
        }

    private:
        void resize(std::size_t newCapacity)
        {
            if(currentCapacity >= newCapacity)
            {
                return;
            }

            EC::Meta::forEach<ComponentsList>([this, newCapacity] (auto t) {
                std::get<std::vector<decltype(t)> >(this->componentsStorage).resize(newCapacity);
            });

            entities.resize(newCapacity);
            for(std::size_t i = currentCapacity; i < newCapacity; ++i)
            {
                entities[i] = std::make_tuple(false, i, BitsetType{});
            }

            currentCapacity = newCapacity;
        }

    public:
        std::size_t addEntity()
        {
            if(currentSize == currentCapacity)
            {
                resize(currentCapacity + EC_GROW_SIZE_AMOUNT);
            }

            std::get<bool>(entities[currentSize]) = true;

            return currentSize++;
        }

        void deleteEntity(std::size_t index)
        {
            std::get<bool>(entities.at(index)) = false;
        }

        bool hasEntity(std::size_t index) const
        {
            return index < currentSize;
        }

        const EntitiesTupleType& getEntityInfo(std::size_t index) const
        {
            return entities.at(index);
        }

        template <typename Component>
        Component& getEntityData(std::size_t index)
        {
            return std::get<std::vector<Component> >(componentsStorage).at(std::get<std::size_t>(entities.at(index)));
        }

        template <typename Component>
        bool hasComponent(std::size_t index) const
        {
            return std::get<BitsetType>(entities.at(index)).template getComponentBit<Component>();
        }

        template <typename Tag>
        bool hasTag(std::size_t index) const
        {
            return std::get<BitsetType>(entities.at(index)).template getTagBit<Tag>();
        }

        void cleanup()
        {
            if(currentSize == 0)
            {
                return;
            }

            std::size_t rhs = currentSize - 1;
            std::size_t lhs = 0;

            while(lhs < rhs)
            {
                while(!std::get<bool>(entities[rhs]))
                {
                    --rhs;
                    if(rhs == 0)
                    {
                        currentSize = 0;
                        return;
                    }
                }
                if(lhs >= rhs)
                {
                    break;
                }
                else if(!std::get<bool>(entities[lhs]))
                {
                    // lhs is marked for deletion
                    // swap lhs entity with rhs entity
                    std::swap(entities[lhs], entities.at(rhs));

                    // clear deleted bitset
                    std::get<BitsetType>(entities[rhs]).reset();

                    // inc/dec pointers
                    ++lhs; --rhs;
                }
                else
                {
                    ++lhs;
                }
            }
            currentSize = rhs + 1;
        }

        template <typename Component, typename... Args>
        void addComponent(std::size_t entityID, Args&&... args)
        {
            if(!hasEntity(entityID))
            {
                return;
            }

            Component component(args...);

            std::get<BitsetType>(entities[entityID]).template getComponentBit<Component>() = true;
            std::get<std::vector<Component> >(componentsStorage)[std::get<std::size_t>(entities[entityID])] = component;
        }

        template <typename Component>
        void removeComponent(std::size_t entityID)
        {
            if(!hasEntity(entityID))
            {
                return;
            }

            std::get<BitsetType>(entities[entityID]).template getComponentBit<Component>() = false;
        }

        template <typename Tag>
        void addTag(std::size_t entityID)
        {
            if(!hasEntity(entityID))
            {
                return;
            }

            std::get<BitsetType>(entities[entityID]).template getTagBit<Tag>() = true;
        }

        template <typename Tag>
        void removeTag(std::size_t entityID)
        {
            if(!hasEntity(entityID))
            {
                return;
            }

            std::get<BitsetType>(entities[entityID]).template getTagBit<Tag>() = false;
        }

    private:
        template <typename... Types>
        struct ForMatchingSignatureHelper
        {
            template <typename CType, typename Function>
            static void call(std::size_t entityID, CType& ctype, Function&& function)
            {
                function(
                    entityID,
                    ctype.template getEntityData<Types>(entityID)...
                );
            }
        };

    public:
        template <typename Signature, typename Function>
        void forMatchingSignature(Function&& function)
        {
            using SignatureComponents = typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper = EC::Meta::Morph<SignatureComponents, ForMatchingSignatureHelper<> >;

            BitsetType signatureBitset = BitsetType::template generateBitset<Signature>();
            for(std::size_t i = 0; i < currentSize; ++i)
            {
                if(!std::get<bool>(entities[i]))
                {
                    continue;
                }

                if((signatureBitset & std::get<BitsetType>(entities[i])) == signatureBitset)
                {
                    Helper::call(i, *this, std::forward<Function>(function));
                }
            }
        }

    };
}

#endif

