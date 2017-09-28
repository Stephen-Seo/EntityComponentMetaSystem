
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_MANAGER_HPP
#define EC_MANAGER_HPP

#define EC_INIT_ENTITIES_SIZE 256
#define EC_GROW_SIZE_AMOUNT 256

#include <cstddef>
#include <vector>
#include <tuple>
#include <utility>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <algorithm>

#include "Meta/Combine.hpp"
#include "Meta/Matching.hpp"
#include "Bitset.hpp"

namespace EC
{
    /*!
        \brief Manages an EntityComponent system.

        EC::Manager must be created with a list of all used Components and all
        used tags.

        Note that all components must have a default constructor.

        Example:
        \code{.cpp}
            EC::Manager<TypeList<C0, C1, C2>, TypeList<T0, T1>> manager;
        \endcode
    */
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
        using ComponentsStorage =
            typename EC::Meta::Morph<ComponentsList, Storage<> >::type;
        // Entity: isAlive, dataIndex, ComponentsTags Info
        using EntitiesTupleType = std::tuple<bool, std::size_t, BitsetType>;
        using EntitiesType = std::vector<EntitiesTupleType>;

        EntitiesType entities;
        ComponentsStorage componentsStorage;
        std::size_t currentCapacity = 0;
        std::size_t currentSize = 0;

    public:
        /*!
            \brief Initializes the manager with a default capacity.

            The default capacity is set with macro EC_INIT_ENTITIES_SIZE,
            and will grow by amounts of EC_GROW_SIZE_AMOUNT when needed.
        */
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
                std::get<std::vector<decltype(t)> >(
                    this->componentsStorage).resize(newCapacity);
            });

            entities.resize(newCapacity);
            for(std::size_t i = currentCapacity; i < newCapacity; ++i)
            {
                entities[i] = std::make_tuple(false, i, BitsetType{});
            }

            currentCapacity = newCapacity;
        }

    public:
        /*!
            \brief Adds an entity to the system, returning the ID of the entity.

            WARNING: The ID of an entity may change after calls to cleanup().
            Usage of entity IDs should be safe during initialization.
            Otherwise, only use the ID given during usage of
            forMatchingSignature().
        */
        std::size_t addEntity()
        {
            if(currentSize == currentCapacity)
            {
                resize(currentCapacity + EC_GROW_SIZE_AMOUNT);
            }

            std::get<bool>(entities[currentSize]) = true;

            return currentSize++;
        }

        /*!
            \brief Marks an entity for deletion.

            A deleted Entity is not actually deleted until cleanup() is called.
            While an Entity is "deleted" but still in the system, calls to
            forMatchingSignature() will ignore the Entity.
        */
        void deleteEntity(const std::size_t& index)
        {
            std::get<bool>(entities.at(index)) = false;
        }


        /*!
            \brief Checks if the Entity with the given ID is in the system.

            Note that deleted Entities that haven't yet been cleaned up
            (via cleanup()) are considered still in the system.
        */
        bool hasEntity(const std::size_t& index) const
        {
            return index < currentSize;
        }


        /*!
            \brief Checks if the Entity is not marked as deleted.

            Note that invalid Entities (Entities where calls to hasEntity()
            returns false) will return false.
        */
        bool isAlive(const std::size_t& index) const
        {
            return hasEntity(index) && std::get<bool>(entities.at(index));
        }

        /*!
            \brief Returns a const reference to an Entity's info.

            An Entity's info is a std::tuple with a bool, std::size_t, and a
            bitset.

            \n The bool determines if the Entity is alive.
            \n The std::size_t is the ID to this Entity's data in the system.
            \n The bitset shows what Components and Tags belong to the Entity.
        */
        const EntitiesTupleType& getEntityInfo(const std::size_t& index) const
        {
            return entities.at(index);
        }

        /*!
            \brief Returns a reference to a component belonging to the given
                Entity.

            This function will return a reference to a Component regardless of
            whether or not the Entity actually owns the reference. If the Entity
            doesn't own the Component, changes to the Component will not affect
            any Entity. It is recommended to use hasComponent() to determine if
            the Entity actually owns that Component.
        */
        template <typename Component>
        Component& getEntityData(const std::size_t& index)
        {
            return std::get<std::vector<Component> >(componentsStorage).at(
                std::get<std::size_t>(entities.at(index)));
        }

        /*!
            \brief Returns a reference to a component belonging to the given
                Entity.

            Note that this function is the same as getEntityData().

            This function will return a reference to a Component regardless of
            whether or not the Entity actually owns the reference. If the Entity
            doesn't own the Component, changes to the Component will not affect
            any Entity. It is recommended to use hasComponent() to determine if
            the Entity actually owns that Component.
        */
        template <typename Component>
        Component& getEntityComponent(const std::size_t& index)
        {
            return getEntityData<Component>(index);
        }

        /*!
            \brief Returns a const reference to a component belonging to the
                given Entity.

            This function will return a const reference to a Component
            regardless of whether or not the Entity actually owns the reference.
            If the Entity doesn't own the Component, changes to the Component
            will not affect any Entity. It is recommended to use hasComponent()
            to determine if the Entity actually owns that Component.
        */
        template <typename Component>
        const Component& getEntityData(const std::size_t& index) const
        {
            return std::get<std::vector<Component> >(componentsStorage).at(
                std::get<std::size_t>(entities.at(index)));
        }

        /*!
            \brief Returns a const reference to a component belonging to the
                given Entity.

            Note that this function is the same as getEntityData() (const).

            This function will return a const reference to a Component
            regardless of whether or not the Entity actually owns the reference.
            If the Entity doesn't own the Component, changes to the Component
            will not affect any Entity. It is recommended to use hasComponent()
            to determine if the Entity actually owns that Component.
        */
        template <typename Component>
        const Component& getEntityComponent(const std::size_t& index) const
        {
            return getEntityData<Component>(index);
        }

        /*!
            \brief Checks whether or not the given Entity has the given
                Component.

            Example:
            \code{.cpp}
                manager.hasComponent<C0>(entityID);
            \endcode
        */
        template <typename Component>
        bool hasComponent(const std::size_t& index) const
        {
            return std::get<BitsetType>(
                entities.at(index)).template getComponentBit<Component>();
        }

        /*!
            \brief Checks whether or not the given Entity has the given Tag.

            Example:
            \code{.cpp}
                manager.hasTag<T0>(entityID);
            \endcode
        */
        template <typename Tag>
        bool hasTag(const std::size_t& index) const
        {
            return std::get<BitsetType>(
                entities.at(index)).template getTagBit<Tag>();
        }

        /*!
            \brief Does garbage collection on Entities.

            Does housekeeping on the vector containing Entities that will
            result in entity IDs changing if some Entities were marked for
            deletion.

            <b>This function should be called periodically to correctly handle
            deletion of entities.</b>

            The map returned by this function lists all entities that have
            changed as a result of calling this function.
            The size_t key refers to the original entity id of the entity,
            the bool value refers to whether or not the entity is still alive,
            and the size_t value refers to the new entity id if the entity
            is still alive.
        */
        using CleanupReturnType =
            std::unordered_map<std::size_t, std::pair<bool, std::size_t> >;
        CleanupReturnType cleanup()
        {
            CleanupReturnType changedMap;
            if(currentSize == 0)
            {
                return changedMap;
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
                        return changedMap;
                    }
                    changedMap.insert(std::make_pair(rhs, std::make_pair(
                        false, 0)));
                    std::get<BitsetType>(entities[rhs]).reset();
                    --rhs;
                }
                if(lhs >= rhs)
                {
                    break;
                }
                else if(!std::get<bool>(entities[lhs]))
                {
                    // lhs is marked for deletion

                    // store deleted and changed id to map
                    changedMap.insert(std::make_pair(lhs, std::make_pair(
                        false, 0)));
                    changedMap.insert(std::make_pair(rhs, std::make_pair(
                        true, lhs)));

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

            return changedMap;
        }

        /*!
            \brief Adds a component to the given Entity.

            Additional parameters given to this function will construct the
            Component with those parameters.

            Note that if the Entity already has the same component, then it
            will be overwritten by the newly created Component with the given
            arguments.

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
        template <typename Component, typename... Args>
        void addComponent(const std::size_t& entityID, Args&&... args)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            Component component(std::forward<Args>(args)...);

            std::get<BitsetType>(
                entities[entityID]
            ).template getComponentBit<Component>() = true;

            std::get<std::vector<Component> >(
                componentsStorage
            )[std::get<std::size_t>(entities[entityID])] = std::move(component);
        }

        /*!
            \brief Removes the given Component from the given Entity.

            If the Entity does not have the Component given, nothing will
            change.

            Example:
            \code{.cpp}
                manager.removeComponent<C0>(entityID);
            \endcode
        */
        template <typename Component>
        void removeComponent(const std::size_t& entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(
                entities[entityID]
            ).template getComponentBit<Component>() = false;
        }

        /*!
            \brief Adds the given Tag to the given Entity.

            Example:
            \code{.cpp}
                manager.addTag<T0>(entityID);
            \endcode
        */
        template <typename Tag>
        void addTag(const std::size_t& entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(
                entities[entityID]
            ).template getTagBit<Tag>() = true;
        }

    /*!
        \brief Removes the given Tag from the given Entity.

        If the Entity does not have the Tag given, nothing will change.

        Example:
        \code{.cpp}
            manager.removeTag<T0>(entityID);
        \endcode
    */
        template <typename Tag>
        void removeTag(const std::size_t& entityID)
        {
            if(!hasEntity(entityID) || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(
                entities[entityID]
            ).template getTagBit<Tag>() = false;
        }

    private:
        template <typename... Types>
        struct ForMatchingSignatureHelper
        {
            template <typename CType, typename Function>
            static void call(
                const std::size_t& entityID,
                CType& ctype,
                Function&& function)
            {
                function(
                    entityID,
                    ctype.template getEntityData<Types>(entityID)...
                );
            }

            template <typename CType, typename Function>
            void callInstance(
                const std::size_t& entityID,
                CType& ctype,
                Function&& function) const
            {
                ForMatchingSignatureHelper<Types...>::call(
                    entityID,
                    ctype,
                    std::forward<Function>(function));
            }
        };

    public:
        /*!
            \brief Calls the given function on all Entities matching the given
                Signature.

            The function object given to this function must accept std::size_t
            as its first parameter and Component references for the rest of the
            parameters. Tags specified in the Signature are only used as
            filters and will not be given as a parameter to the function.

            Example:
            \code{.cpp}
                manager.forMatchingSignature<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                });
            \endcode
            Note, the ID given to the function is not permanent. An entity's ID
            may change when cleanup() is called.
        */
        template <typename Signature, typename Function>
        void forMatchingSignature(Function&& function)
        {
            using SignatureComponents =
                typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper =
                EC::Meta::Morph<
                    SignatureComponents,
                    ForMatchingSignatureHelper<> >;

            BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();
            for(std::size_t i = 0; i < currentSize; ++i)
            {
                if(!std::get<bool>(entities[i]))
                {
                    continue;
                }

                if((signatureBitset & std::get<BitsetType>(entities[i]))
                    == signatureBitset)
                {
                    Helper::call(i, *this, std::forward<Function>(function));
                }
            }
        }

    private:
        std::unordered_map<std::size_t, std::function<void()> >
            forMatchingFunctions;
        std::size_t functionIndex = 0;

    public:
        /*!
            \brief Stores a function in the manager to be called later.

            As an alternative to calling functions directly with
            forMatchingSignature(), functions can be stored in the manager to
            be called later with callForMatchingFunctions() and
            callForMatchingFunction, and removed with
            clearForMatchingFunctions() and removeForMatchingFunction().

            The syntax for the Function is the same as with
            forMatchingSignature().

            Note that functions will be called in the same order they are
            inserted if called by callForMatchingFunctions() unless the
            internal functionIndex counter has wrapped around (is a
            std::size_t). Calling clearForMatchingFunctions() will reset this
            counter to zero.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode

            \return The index of the function, used for deletion with
                deleteForMatchingFunction() or filtering with
                keepSomeMatchingFunctions() or removeSomeMatchingFunctions(),
                or calling with callForMatchingFunction().
        */
        template <typename Signature, typename Function>
        std::size_t addForMatchingFunction(Function&& function)
        {
            while(forMatchingFunctions.find(functionIndex)
                != forMatchingFunctions.end())
            {
                ++functionIndex;
            }

            using SignatureComponents =
                typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper =
                EC::Meta::Morph<
                    SignatureComponents,
                    ForMatchingSignatureHelper<> >;

            Helper helper;
            BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();

            forMatchingFunctions.emplace(std::make_pair(
                functionIndex,
                [function, signatureBitset, helper, this] ()
            {
                for(std::size_t i = 0; i < this->currentSize; ++i)
                {
                    if(!std::get<bool>(this->entities[i]))
                    {
                        continue;
                    }
                    if((signatureBitset
                        & std::get<BitsetType>(this->entities[i]))
                            == signatureBitset)
                    {
                        helper.callInstance(i, *this, function);
                    }
                }
            }));

            return functionIndex++;
        }

        /*!
            \brief Call all stored functions.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode
        */
        void callForMatchingFunctions()
        {
            for(auto functionIter = forMatchingFunctions.begin();
                functionIter != forMatchingFunctions.end();
                ++functionIter)
            {
                functionIter->second();
            }
        }

        /*!
            \brief Call a specific stored function.

            Example:
            \code{.cpp}
                std::size_t id =
                    manager.addForMatchingFunction<TypeList<C0, C1, T0>>(
                        [] (std::size_t ID, C0& c0, C1& c1) {
                    // Lambda function contents here
                });

                // call the previously added function
                manager.callForMatchingFunction(id);
            \endcode

            \return False if a function with the given id does not exist.
        */
        bool callForMatchingFunction(std::size_t id)
        {
            auto iter = forMatchingFunctions.find(id);
            if(iter == forMatchingFunctions.end())
            {
                return false;
            }
            iter->second();
            return true;
        }

        /*!
            \brief Remove all stored functions.

            Also resets the index counter of stored functions to 0.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode
        */
        void clearForMatchingFunctions()
        {
            forMatchingFunctions.clear();
            functionIndex = 0;
        }

        /*!
            \brief Removes a function that has the given id.

            \return True if a function was erased.
        */
        bool removeForMatchingFunction(std::size_t id)
        {
            return forMatchingFunctions.erase(id) == 1;
        }

        /*!
            \brief Removes all functions that do not have the index specified
                in argument "list".

            The given List must be iterable.
            This is the only requirement, so a set could also be given.

            \return The number of functions deleted.
        */
        template <typename List>
        std::size_t keepSomeMatchingFunctions(List list)
        {
            std::size_t deletedCount = 0;
            for(auto iter = forMatchingFunctions.begin();
                iter != forMatchingFunctions.end();)
            {
                if(std::find(list.begin(), list.end(), iter->first)
                    == list.end())
                {
                    iter = forMatchingFunctions.erase(iter);
                    ++deletedCount;
                }
                else
                {
                    ++iter;
                }
            }

            return deletedCount;
        }

        /*!
            \brief Removes all functions that do not have the index specified
                in argument "list".

            This function allows for passing an initializer list.

            \return The number of functions deleted.
        */
        std::size_t keepSomeMatchingFunctions(
            std::initializer_list<std::size_t> list)
        {
            return keepSomeMatchingFunctions<decltype(list)>(list);
        }

        /*!
            \brief Removes all functions that do have the index specified
                in argument "list".

            The given List must be iterable.
            This is the only requirement, so a set could also be given.

            \return The number of functions deleted.
        */
        template <typename List>
        std::size_t removeSomeMatchingFunctions(List list)
        {
            std::size_t deletedCount = 0;
            for(auto listIter = list.begin();
                listIter != list.end();
                ++listIter)
            {
                deletedCount += forMatchingFunctions.erase(*listIter);
            }

            return deletedCount;
        }

        /*!
            \brief Removes all functions that do have the index specified
                in argument "list".

            This function allows for passing an initializer list.

            \return The number of functions deleted.
        */
        std::size_t removeSomeMatchingFunctions(
            std::initializer_list<std::size_t> list)
        {
            return removeSomeMatchingFunctions<decltype(list)>(list);
        }

        /*!
            \brief Deletes the specified function.

            The index of a function is returned from addForMatchingFunction()
                so there is no other way to get the index of a function.

            \return True if function existed and has been deleted.
        */
        bool deleteForMatchingFunction(std::size_t index)
        {
            return forMatchingFunctions.erase(index) == 1;
        }

        /*!
            \brief Resets the Manager, removing all entities.

            Some data may persist but will be overwritten when new entities
            are added. Thus, do not depend on data to persist after a call to
            reset().
        */
        void reset()
        {
            clearForMatchingFunctions();

            currentSize = 0;
            currentCapacity = 0;
            resize(EC_INIT_ENTITIES_SIZE);
        }
    };
}

#endif

