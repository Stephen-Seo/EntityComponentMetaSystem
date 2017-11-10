
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
#include <thread>
#include <queue>
#include <mutex>

#ifndef NDEBUG
  #include <iostream>
#endif

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
            \brief Returns the current size or number of entities in the system.

            Note that this size includes entities that have been marked for
            deletion but not yet cleaned up by the cleanup function.
        */
        std::size_t getCurrentSize() const
        {
            return currentSize;
        }

        /*
            \brief Returns the current capacity or number of entities the system
                can hold.

            Note that when capacity is exceeded, the capacity is increased by
            EC_GROW_SIZE_AMOUNT.
        */
        std::size_t getCurrentCapacity() const
        {
            return currentCapacity;
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

            The queue returned by this function lists all entities that have
            changed as a result of calling this function.

            The first parameter in the tuple (bool) is true if the entity has
            been relocated (entity id has changed) and is false if the entity
            has been deleted.

            The second parameter (size_t) is the original entity id of the
            entity. If the previous parameter was false for deleted, then this
            refers to the id of that deleted entity. If the previous parameter
            was true for relocated, then this refers to the previous id of the
            relocated entity

            The third parameter (size_t) is the new entity id of the entity if
            the entity was relocated. If the entity was deleted then this
            parameter is undefined as it is not used in that case.
        */
        using CleanupReturnType =
            std::queue<std::tuple<bool, std::size_t, std::size_t> >;
        CleanupReturnType cleanup()
        {
            CleanupReturnType changedQueue;
            if(currentSize == 0)
            {
                return changedQueue;
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
                        return changedQueue;
                    }
                    changedQueue.push(std::make_tuple(false, rhs, 0));
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

                    // store deleted and changed id to queue
                    changedQueue.push(std::make_tuple(false, lhs, 0));
                    changedQueue.push(std::make_tuple(true, rhs, lhs));

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

            return changedQueue;
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

            The second parameter is default 1 (not multi-threaded). If the
            second parameter threadCount is set to a value greater than 1, then
            threadCount threads will be used.
            Note that multi-threading is based on splitting the task of calling
            the function across sections of entities. Thus if there are only
            a small amount of entities in the manager, then using multiple
            threads may not have as great of a speed-up.

            Example:
            \code{.cpp}
                manager.forMatchingSignature<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                },
                4 // four threads
                );
            \endcode
            Note, the ID given to the function is not permanent. An entity's ID
            may change when cleanup() is called.
        */
        template <typename Signature, typename Function>
        void forMatchingSignature(Function&& function,
            std::size_t threadCount = 1)
        {
            using SignatureComponents =
                typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper =
                EC::Meta::Morph<
                    SignatureComponents,
                    ForMatchingSignatureHelper<> >;

            BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();
            if(threadCount <= 1)
            {
                for(std::size_t i = 0; i < currentSize; ++i)
                {
                    if(!std::get<bool>(entities[i]))
                    {
                        continue;
                    }

                    if((signatureBitset & std::get<BitsetType>(entities[i]))
                        == signatureBitset)
                    {
                        Helper::call(i, *this,
                            std::forward<Function>(function));
                    }
                }
            }
            else
            {
                std::vector<std::thread> threads(threadCount);
                std::size_t s = currentSize / threadCount;
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == threadCount - 1)
                    {
                        end = currentSize;
                    }
                    else
                    {
                        end = s * (i + 1);
                    }
                    threads[i] = std::thread([this, &function, &signatureBitset]
                            (std::size_t begin,
                            std::size_t end) {
                        for(std::size_t i = begin; i < end; ++i)
                        {
                            if(!std::get<bool>(this->entities[i]))
                            {
                                continue;
                            }

                            if((signatureBitset
                                    & std::get<BitsetType>(entities[i]))
                                == signatureBitset)
                            {
                                Helper::call(i, *this,
                                    std::forward<Function>(function));
                            }
                        }
                    },
                        begin,
                        end);
                }
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    threads[i].join();
                }
            }
        }

    private:
        std::unordered_map<std::size_t, std::function<void(std::size_t)> >
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
                [function, signatureBitset, helper, this] 
                    (std::size_t threadCount)
            {
                if(threadCount <= 1)
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
                }
                else
                {
                    std::vector<std::thread> threads(threadCount);
                    std::size_t s = this->currentSize / threadCount;
                    for(std::size_t i = 0; i < threadCount; ++ i)
                    {
                        std::size_t begin = s * i;
                        std::size_t end;
                        if(i == threadCount - 1)
                        {
                            end = this->currentSize;
                        }
                        else
                        {
                            end = s * (i + 1);
                        }
                        threads[i] = std::thread(
                            [this, &function, &signatureBitset, &helper]
                                (std::size_t begin,
                                std::size_t end) {
                            for(std::size_t i = begin; i < end; ++i)
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
                        },
                        begin, end);
                    }
                    for(std::size_t i = 0; i < threadCount; ++i)
                    {
                        threads[i].join();
                    }
                }
            }));

            return functionIndex++;
        }

        /*!
            \brief Call all stored functions.

            A second parameter can be optionally used to specify the number
            of threads to use when calling the functions. Otherwise, this
            function is by default not multi-threaded.
            Note that multi-threading is based on splitting the task of calling
            the functions across sections of entities. Thus if there are only
            a small amount of entities in the manager, then using multiple
            threads may not have as great of a speed-up.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([] (
                    std::size_t ID, C0& component0, C1& component1) {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // call all stored functions with 4 threads
                manager.callForMatchingFunctions(4);

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode
        */
        void callForMatchingFunctions(std::size_t threadCount = 1)
        {
            for(auto functionIter = forMatchingFunctions.begin();
                functionIter != forMatchingFunctions.end();
                ++functionIter)
            {
                functionIter->second(threadCount);
            }
        }

        /*!
            \brief Call a specific stored function.

            A second parameter can be optionally used to specify the number
            of threads to use when calling the function. Otherwise, this
            function is by default not multi-threaded.
            Note that multi-threading is based on splitting the task of calling
            the function across sections of entities. Thus if there are only
            a small amount of entities in the manager, then using multiple
            threads may not have as great of a speed-up.

            Example:
            \code{.cpp}
                std::size_t id =
                    manager.addForMatchingFunction<TypeList<C0, C1, T0>>(
                        [] (std::size_t ID, C0& c0, C1& c1) {
                    // Lambda function contents here
                });

                // call the previously added function
                manager.callForMatchingFunction(id);

                // call the previously added function with 4 threads
                manager.callForMatchingFunction(id, 4);
            \endcode

            \return False if a function with the given id does not exist.
        */
        bool callForMatchingFunction(std::size_t id,
            std::size_t threadCount = 1)
        {
            auto iter = forMatchingFunctions.find(id);
            if(iter == forMatchingFunctions.end())
            {
                return false;
            }
            iter->second(threadCount);
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
            \brief Call multiple functions with mulitple signatures on all
                living entities.

            (Living entities as in entities that have not been marked for
            deletion.)

            This function requires the first template parameter to be a
            EC::Meta::TypeList of signatures. Note that a signature is a
            EC::Meta::TypeList of components and tags, meaning that SigList
            is a TypeList of TypeLists.

            The second template parameter can be inferred from the function
            parameter which should be a tuple of functions. The function
            at any index in the tuple should match with a signature of the
            same index in the SigList. Behavior is undefined if there are
            less functions than signatures.

            See the Unit Test of this function in src/test/ECTest.cpp for
            usage examples.

            This function was created for the use case where there are many
            entities in the system which can cause multiple calls to
            forMatchingSignature to be slow due to the overhead of iterating
            through the entire list of entities on each invocation.
            This function instead iterates through all entities once,
            storing matching entities in a vector of vectors (for each
            signature and function pair) and then calling functions with
            the matching list of entities.

            Note that multi-threaded or not, functions will be called in order
            of signatures. The first function signature pair will be called
            first, then the second, third, and so on.
            If this function is called with more than 1 thread specified, then
            the order of entities called is not guaranteed. Otherwise entities
            will be called in consecutive order by their ID.
        */
        template <typename SigList, typename FuncTuple>
        void forMatchingSignatures(FuncTuple funcTuple,
            std::size_t threadCount = 1)
        {
            std::vector<std::vector<std::size_t> > multiMatchingEntities(
                SigList::size);
            BitsetType signatureBitsets[SigList::size];

            // generate bitsets for each signature
            EC::Meta::forEach<SigList>(
            [this, &signatureBitsets] (auto signature) {
                signatureBitsets[
                        EC::Meta::IndexOf<decltype(signature), SigList>{} ] =
                    BitsetType::template generateBitset<decltype(signature)>();
            });

            // find and store entities matching signatures
            if(threadCount <= 1)
            {
                for(std::size_t eid = 0; eid < currentSize; ++eid)
                {
                    if(!isAlive(eid))
                    {
                        continue;
                    }
                    for(std::size_t i = 0; i < SigList::size; ++i)
                    {
                        if((signatureBitsets[i]
                            & std::get<BitsetType>(entities[eid]))
                                == signatureBitsets[i])
                        {
                            multiMatchingEntities[i].push_back(eid);
                        }
                    }
                }
            }
            else
            {
                std::vector<std::thread> threads(threadCount);
                std::size_t s = currentSize / threadCount;
                std::mutex sigsMutexes[SigList::size];
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == threadCount - 1)
                    {
                        end = currentSize;
                    }
                    else
                    {
                        end = s * (i + 1);
                    }
                    threads[i] = std::thread(
                    [this, &signatureBitsets, &multiMatchingEntities,
                        &sigsMutexes]
                    (std::size_t begin, std::size_t end) {
                        for(std::size_t eid = begin; eid < end; ++eid)
                        {
                            if(!isAlive(eid))
                            {
                                continue;
                            }
                            for(std::size_t i = 0; i < SigList::size; ++i)
                            {
                                if((signatureBitsets[i]
                                    & std::get<BitsetType>(entities[eid]))
                                        == signatureBitsets[i])
                                {
                                    std::lock_guard<std::mutex> guard(
                                        sigsMutexes[i]);
                                    multiMatchingEntities[i].push_back(eid);

                                }
                            }
                        }
                    },
                    begin, end);
                }
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    threads[i].join();
                }
            }

            // call functions on matching entities
            EC::Meta::forEach<SigList>(
            [this, &multiMatchingEntities, &funcTuple, &threadCount]
            (auto signature) {
                using SignatureComponents =
                    typename EC::Meta::Matching<
                        decltype(signature), ComponentsList>::type;
                using Helper =
                    EC::Meta::Morph<
                        SignatureComponents,
                        ForMatchingSignatureHelper<> >;
                using Index = EC::Meta::IndexOf<decltype(signature),
                    SigList>;
                constexpr std::size_t index = Index{};
                if(threadCount <= 1)
                {
                    for(auto iter = multiMatchingEntities[index].begin();
                        iter != multiMatchingEntities[index].end(); ++iter)
                    {
                        Helper::call(*iter, *this,
                            std::get<index>(funcTuple));
                    }
                }
                else
                {
                    std::vector<std::thread> threads(threadCount);
                    std::size_t s = multiMatchingEntities[index].size()
                        / threadCount;
                    for(std::size_t i = 0; i < threadCount; ++i)
                    {
                        std::size_t begin = s * i;
                        std::size_t end;
                        if(i == threadCount - 1)
                        {
                            end = multiMatchingEntities[index].size();
                        }
                        else
                        {
                            end = s * (i + 1);
                        }
                        threads[i] = std::thread(
                        [this, &multiMatchingEntities, &funcTuple]
                        (std::size_t begin, std::size_t end)
                        {
                            for(std::size_t j = begin; j < end; ++j)
                            {
                                Helper::call(multiMatchingEntities[index][j],
                                    *this, std::get<index>(funcTuple));
                            }
                        }, begin, end);
                    }
                    for(std::size_t i = 0; i < threadCount; ++i)
                    {
                        threads[i].join();
                    }
                }
            });
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

