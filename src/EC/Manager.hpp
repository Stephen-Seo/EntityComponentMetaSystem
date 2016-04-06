
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


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

        bool isAlive(std::size_t index) const
        {
            return hasEntity(index) && std::get<bool>(entities.at(index));
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
                    if(rhs == 0)
                    {
                        currentSize = 0;
                        return;
                    }
                    --rhs;
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
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            Component component(std::forward<Args>(args)...);

            std::get<BitsetType>(entities[entityID]).template getComponentBit<Component>() = true;
            std::get<std::vector<Component> >(componentsStorage)[std::get<std::size_t>(entities[entityID])] = std::move(component);
        }

        template <typename Component>
        void removeComponent(std::size_t entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(entities[entityID]).template getComponentBit<Component>() = false;
        }

        template <typename Tag>
        void addTag(std::size_t entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(entities[entityID]).template getTagBit<Tag>() = true;
        }

        template <typename Tag>
        void removeTag(std::size_t entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
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

/*! \class EC::Manager
    \brief Manages an EntityComponent system.

    EC::Manager must be created with a list of all used Components and all used tags.

    Note that all components must have a default constructor.

    Example:
    \code{.cpp}
        EC::Manager<TypeList<C0, C1, C2>, TypeList<T0, T1>> manager;
    \endcode
*/

/*! \fn EC::Manager::Manager()
    \brief Initializes the manager with a default capacity.

    The default capacity is set with macro EC_INIT_ENTITIES_SIZE,
    and will grow by amounts of EC_GROW_SIZE_AMOUNT when needed.
*/

/*! \fn std::size_t EC::Manager::addEntity()
    \brief Adds an entity to the system, returning the ID of the entity.

    WARNING: The ID of an entity may change after calls to cleanup().
    Usage of entity IDs should be safe during initialization.
    Otherwise, only use the ID given during usage of forMatchingSignature().
*/

/*! \fn void EC::Manager::deleteEntity(std::size_t index)
    \brief Marks an entity for deletion.

    A deleted Entity is not actually deleted until cleanup() is called.
    While an Entity is "deleted" but still in the system, calls to forMatchingSignature()
    will ignore the Entity.
*/

/*! \fn bool EC::Manager::hasEntity(std::size_t index) const
    \brief Checks if the Entity with the given ID is in the system.

    Note that deleted Entities that haven't yet been cleaned up (via cleanup()) are
    considered still in the system.
*/

/*! \fn bool EC::Manager::isAlive(std::size_t index) const
    \brief Checks if the Entity is not marked as deleted.

    Note that invalid Entities (Entities where calls to hasEntity() returns false)
    will return false.
*/

/*! \fn const EntitiesTupleType& EC::Manager::getEntityInfo(std::size_t index) const
    \brief Returns a const reference to an Entity's info.

    An Entity's info is a std::tuple with a bool, std::size_t, and a bitset.
    \n The bool determines if the Entity is alive.
    \n The std::size_t is the ID to this Entity's data in the system.
    \n The bitset shows what Components and Tags belong to the Entity.
*/

/*! \fn Component& EC::Manager::getEntityData(std::size_t index)
    \brief Returns a reference to a component belonging to the given Entity.

    This function will return a reference to a Component regardless of whether or
    not the Entity actually owns the reference. If the Entity doesn't own the Component,
    changes to the Component will not affect any Entity. It is recommended to use
    hasComponent() to determine if the Entity actually owns that Component.
*/

/*! \fn bool EC::Manager::hasComponent(std::size_t index) const
    \brief Checks whether or not the given Entity has the given Component.

    Example:
    \code{.cpp}
        manager.hasComponent<C0>(entityID);
    \endcode
*/

/*! \fn bool EC::Manager::hasTag(std::size_t index) const
    \brief Checks whether or not the given Entity has the given Tag.

    Example:
    \code{.cpp}
        manager.hasTag<T0>(entityID);
    \endcode
*/

/*! \fn void EC::Manager::cleanup()
    \brief Does garbage collection on Entities.

    Does housekeeping on the vector containing Entities that will result in
    entity IDs changing if some Entities were marked for deletion.
    <b>This function should be called periodically to correctly handle deletion of entities.</b>
*/

/*! \fn void EC::Manager::addComponent(std::size_t entityID, Args&&... args)
    \brief Adds a component to the given Entity.

    Additional parameters given to this function will construct the Component with those
    parameters.

    Note that if the Entity already has the same component, then it will be overwritten
    by the newly created Component with the given arguments.

    Example:
    \code{.cpp}
        struct C0
        {
            // constructor is compatible as a default constructor
            C0(int a = 0, char b = 'b') :
            a(a), b(b)
            {}

            int a;
            char b;
        }

        manager.addComponent<C0>(entityID, 10, 'd');
    \endcode
*/

/*! \fn void EC::Manager::removeComponent(std::size_t entityID)
    \brief Removes the given Component from the given Entity.

    If the Entity does not have the Component given, nothing will change.

    Example:
    \code{.cpp}
        manager.removeComponent<C0>(entityID);
    \endcode
*/

/*! \fn void EC::Manager::addTag(std::size_t entityID)
    \brief Adds the given Tag to the given Entity.

    Example:
    \code{.cpp}
        manager.addTag<T0>(entityID);
    \endcode
*/

/*! \fn void EC::Manager::removeTag(std::size_t entityID)
    \brief Removes the given Tag from the given Entity.

    If the Entity does not have the Tag given, nothing will change.

    Example:
    \code{.cpp}
        manager.removeTag<T0>(entityID);
    \endcode
*/

/*! \fn void EC::Manager::forMatchingSignature(Function&& function)
    \brief Calls the given function on all Entities matching the given Signature.

    The function object given to this function must accept std::size_t as its first
    parameter and Component references for the rest of the parameters. Tags specified in the
    Signature are only used as filters and will not be given as a parameter to the function.

    Example:
    \code{.cpp}
        manager.forMatchingSignature<TypeList<C0, C1, T0>>([] (std::size_t ID, C0& component0, C1& component1) {
            // Lambda function contents here.
        });
    \endcode
    Note, the ID given to the function is not permanent. An entity's ID may change when cleanup() is called.
*/

