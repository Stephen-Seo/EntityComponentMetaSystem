
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
#include <mutex>

#ifndef NDEBUG
  #include <iostream>
#endif

#include "Meta/Combine.hpp"
#include "Meta/Matching.hpp"
#include "Meta/ForEachWithIndex.hpp"
#include "Meta/ForEachDoubleTuple.hpp"
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
        using Components = ComponentsList;
        using Tags = TagsList;
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
        // Entity: isAlive, ComponentsTags Info
        using EntitiesTupleType = std::tuple<bool, BitsetType>;
        using EntitiesType = std::vector<EntitiesTupleType>;

        EntitiesType entities;
        ComponentsStorage componentsStorage;
        std::size_t currentCapacity = 0;
        std::size_t currentSize = 0;
        std::unordered_set<std::size_t> deletedSet;

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
                entities[i] = std::make_tuple(false, BitsetType{});
            }

            currentCapacity = newCapacity;
        }

    public:
        /*!
            \brief Adds an entity to the system, returning the ID of the entity.

            Note: The ID of an entity is guaranteed to not change.
        */
        std::size_t addEntity()
        {
            if(deletedSet.empty())
            {
                if(currentSize == currentCapacity)
                {
                    resize(currentCapacity + EC_GROW_SIZE_AMOUNT);
                }

                std::get<bool>(entities[currentSize]) = true;

                return currentSize++;
            }
            else
            {
                std::size_t id;
                {
                    auto iter = deletedSet.begin();
                    id = *iter;
                    deletedSet.erase(iter);
                }
                std::get<bool>(entities[id]) = true;
                return id;
            }
        }

        /*!
            \brief Marks an entity for deletion.

            A deleted Entity's id is stored to be reclaimed later when
            addEntity is called. Thus calling addEntity may return an id of
            a previously deleted Entity.
        */
        void deleteEntity(const std::size_t& index)
        {
            if(hasEntity(index))
            {
                std::get<bool>(entities.at(index)) = false;
                std::get<BitsetType>(entities.at(index)).reset();
                deletedSet.insert(index);
            }
        }


        /*!
            \brief Checks if the Entity with the given ID is in the system.

            Note that deleted Entities are still considered in the system.
            Consider using isAlive().
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

            Note this function will only count entities where isAlive() returns
            true.
        */
        std::size_t getCurrentSize() const
        {
            return currentSize - deletedSet.size();
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

            An Entity's info is a std::tuple with a bool, and a
            bitset.

            \n The bool determines if the Entity is alive.
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
                index);
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
                index);
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
            if(!isAlive(entityID))
            {
                return;
            }

            Component component(std::forward<Args>(args)...);

            std::get<BitsetType>(
                entities[entityID]
            ).template getComponentBit<Component>() = true;

            std::get<std::vector<Component> >(
                componentsStorage
            )[entityID] = std::move(component);
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
            if(!isAlive(entityID))
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
            if(!isAlive(entityID))
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
            if(!isAlive(entityID))
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
            static void callPtr(
                const std::size_t& entityID,
                CType& ctype,
                Function* function)
            {
                (*function)(
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

            template <typename CType, typename Function>
            void callInstancePtr(
                const std::size_t& entityID,
                CType& ctype,
                Function* function) const
            {
                ForMatchingSignatureHelper<Types...>::callPtr(
                    entityID,
                    ctype,
                    function);
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

        /*!
            \brief Calls the given function on all Entities matching the given
                Signature.

            The function pointer given to this function must accept std::size_t
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
                auto function = [] (std::size_t ID, C0& component0,
                    C1& component1) {
                    // Lambda function contents here
                };
                manager.forMatchingSignaturePtr<TypeList<C0, C1, T0>>(
                    &function, // ptr
                    4 // four threads
                );
            \endcode
            Note, the ID given to the function is not permanent. An entity's ID
            may change when cleanup() is called.
        */
        template <typename Signature, typename Function>
        void forMatchingSignaturePtr(Function* function,
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
                        Helper::callPtr(i, *this, function);
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
                                Helper::callPtr(i, *this, function);
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
        std::unordered_map<std::size_t, std::tuple<
            BitsetType,
            std::function<void(
                std::size_t,
                std::vector<std::size_t>)> > >
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
                std::make_tuple(
                    signatureBitset,
                    [function, helper, this] 
                    (std::size_t threadCount,
                        std::vector<std::size_t> matching)
                {
                    if(threadCount <= 1)
                    {
                        for(auto eid : matching)
                        {
                            if(isAlive(eid))
                            {
                                helper.callInstancePtr(eid, *this, &function);
                            }
                        }
                    }
                    else
                    {
                        std::vector<std::thread> threads(threadCount);
                        std::size_t s = matching.size() / threadCount;
                        for(std::size_t i = 0; i < threadCount; ++ i)
                        {
                            std::size_t begin = s * i;
                            std::size_t end;
                            if(i == threadCount - 1)
                            {
                                end = matching.size();
                            }
                            else
                            {
                                end = s * (i + 1);
                            }
                            threads[i] = std::thread(
                                [this, &function, &helper]
                                    (std::size_t begin,
                                    std::size_t end) {
                                for(std::size_t i = begin; i < end; ++i)
                                {
                                    if(isAlive(i))
                                    {
                                        helper.callInstancePtr(i, *this, &function);
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
                })));

            return functionIndex++;
        }

    private:
        std::vector<std::vector<std::size_t> > getMatchingEntities(
            std::vector<BitsetType*> bitsets, std::size_t threadCount = 1)
        {
            std::vector<std::vector<std::size_t> > matchingV(bitsets.size());

            if(threadCount <= 1)
            {
                for(std::size_t i = 0; i < currentSize; ++i)
                {
                    if(!isAlive(i))
                    {
                        continue;
                    }
                    for(std::size_t j = 0; j < bitsets.size(); ++j)
                    {
                        if(((*bitsets[j]) & std::get<BitsetType>(entities[i]))
                            == (*bitsets[j]))
                        {
                            matchingV[j].push_back(i);
                        }
                    }
                }
            }
            else
            {
                std::vector<std::thread> threads(threadCount);
                std::size_t s = currentSize / threadCount;
                std::mutex mutex;
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
                    [this, &matchingV, &bitsets, &mutex]
                    (std::size_t begin, std::size_t end)
                    {
                        for(std::size_t j = begin; j < end; ++j)
                        {
                            if(!isAlive(j))
                            {
                                continue;
                            }
                            for(std::size_t k = 0; k < bitsets.size(); ++k)
                            {
                                if(((*bitsets[k]) &
                                    std::get<BitsetType>(entities[j]))
                                    == (*bitsets[k]))
                                {
                                    std::lock_guard<std::mutex> guard(mutex);
                                    matchingV[k].push_back(j);
                                }
                            }
                        }
                    }, begin, end);
                }
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    threads[i].join();
                }
            }

            return matchingV;
        }

    public:

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
            std::vector<BitsetType*> bitsets;
            for(auto iter = forMatchingFunctions.begin();
                iter != forMatchingFunctions.end();
                ++iter)
            {
                bitsets.push_back(&std::get<BitsetType>(iter->second));
            }

            std::vector<std::vector<std::size_t> > matching =
                getMatchingEntities(bitsets, threadCount);

            std::size_t i = 0;
            for(auto iter = forMatchingFunctions.begin();
                iter != forMatchingFunctions.end();
                ++iter)
            {
                std::get<1>(iter->second)(threadCount, matching[i++]);
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
            std::vector<std::vector<std::size_t> > matching =
                getMatchingEntities(std::vector<BitsetType*>{
                    &std::get<BitsetType>(iter->second)}, threadCount);
            std::get<1>(iter->second)(threadCount, matching[0]);
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
        template <typename SigList, typename FTuple>
        void forMatchingSignatures(
            FTuple fTuple, const std::size_t threadCount = 1)
        {
            std::vector<std::vector<std::size_t> > multiMatchingEntities(
                SigList::size);
            BitsetType signatureBitsets[SigList::size];

            // generate bitsets for each signature
            EC::Meta::forEachWithIndex<SigList>(
            [this, &signatureBitsets] (auto signature, const auto index) {
                signatureBitsets[index] =
                    BitsetType::template generateBitset
                        <decltype(signature)>();
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
                std::mutex mutexes[SigList::size];
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
                    threads[i] = std::thread(
                    [this, &mutexes, &multiMatchingEntities, &signatureBitsets]
                    (std::size_t begin, std::size_t end)
                    {
                        for(std::size_t j = begin; j < end; ++j)
                        {
                            if(!isAlive(j))
                            {
                                continue;
                            }
                            for(std::size_t k = 0; k < SigList::size; ++k)
                            {
                                if((signatureBitsets[k]
                                    & std::get<BitsetType>(entities[j]))
                                        == signatureBitsets[k])
                                {
                                    std::lock_guard<std::mutex> guard(
                                        mutexes[k]);
                                    multiMatchingEntities[k].push_back(j);
                                }
                            }
                        }
                    }, begin, end);
                }
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    threads[i].join();
                }
            }
            
            // call functions on matching entities
            EC::Meta::forEachDoubleTuple(
                EC::Meta::Morph<SigList, std::tuple<> >{},
                fTuple,
                [this, &multiMatchingEntities, &threadCount]
                (auto sig, auto func, auto index)
                {
                    using SignatureComponents =
                        typename EC::Meta::Matching<
                            decltype(sig), ComponentsList>::type;
                    using Helper =
                        EC::Meta::Morph<
                            SignatureComponents,
                            ForMatchingSignatureHelper<> >;
                    if(threadCount <= 1)
                    {
                        for(const auto& id : multiMatchingEntities[index])
                        {
                            if(isAlive(id))
                            {
                                Helper::call(id, *this, func);
                            }
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
                            [this, &multiMatchingEntities, &index, &func]
                            (std::size_t begin, std::size_t end)
                            {
                                for(std::size_t j = begin; j < end;
                                    ++j)
                                {
                                    if(isAlive(multiMatchingEntities[index][j]))
                                    {
                                        Helper::call(
                                            multiMatchingEntities[index][j],
                                            *this,
                                            func);
                                    }
                                }
                            }, begin, end);
                        }
                        for(std::size_t i = 0; i < threadCount; ++i)
                        {
                            threads[i].join();
                        }
                    }
                }
            );
        }

        /*!
            \brief Call multiple functions with mulitple signatures on all
                living entities.

            (Living entities as in entities that have not been marked for
            deletion.)

            Note that this function requires the tuple of functions to hold
            pointers to functions, not just functions.

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
        template <typename SigList, typename FTuple>
        void forMatchingSignaturesPtr(FTuple fTuple,
            std::size_t threadCount = 1)
        {
            std::vector<std::vector<std::size_t> > multiMatchingEntities(
                SigList::size);
            BitsetType signatureBitsets[SigList::size];

            // generate bitsets for each signature
            EC::Meta::forEachWithIndex<SigList>(
            [this, &signatureBitsets] (auto signature, const auto index) {
                signatureBitsets[index] =
                    BitsetType::template generateBitset
                        <decltype(signature)>();
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
                std::mutex mutexes[SigList::size];
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
                    threads[i] = std::thread(
                    [this, &mutexes, &multiMatchingEntities, &signatureBitsets]
                    (std::size_t begin, std::size_t end)
                    {
                        for(std::size_t j = begin; j < end; ++j)
                        {
                            if(!isAlive(j))
                            {
                                continue;
                            }
                            for(std::size_t k = 0; k < SigList::size; ++k)
                            {
                                if((signatureBitsets[k]
                                    & std::get<BitsetType>(entities[j]))
                                        == signatureBitsets[k])
                                {
                                    std::lock_guard<std::mutex> guard(
                                        mutexes[k]);
                                    multiMatchingEntities[k].push_back(j);
                                }
                            }
                        }
                    }, begin, end);
                }
                for(std::size_t i = 0; i < threadCount; ++i)
                {
                    threads[i].join();
                }
            }
            
            // call functions on matching entities
            EC::Meta::forEachDoubleTuple(
                EC::Meta::Morph<SigList, std::tuple<> >{},
                fTuple,
                [this, &multiMatchingEntities, &threadCount]
                (auto sig, auto func, auto index)
                {
                    using SignatureComponents =
                        typename EC::Meta::Matching<
                            decltype(sig), ComponentsList>::type;
                    using Helper =
                        EC::Meta::Morph<
                            SignatureComponents,
                            ForMatchingSignatureHelper<> >;
                    if(threadCount <= 1)
                    {
                        for(const auto& id : multiMatchingEntities[index])
                        {
                            if(isAlive(id))
                            {
                                Helper::callPtr(id, *this, func);
                            }
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
                            [this, &multiMatchingEntities, &index, &func]
                            (std::size_t begin, std::size_t end)
                            {
                                for(std::size_t j = begin; j < end;
                                    ++j)
                                {
                                    if(isAlive(multiMatchingEntities[index][j]))
                                    {
                                        Helper::callPtr(
                                            multiMatchingEntities[index][j],
                                            *this,
                                            func);
                                    }
                                }
                            }, begin, end);
                        }
                        for(std::size_t i = 0; i < threadCount; ++i)
                        {
                            threads[i].join();
                        }
                    }
                }
            );
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
            deletedSet.clear();
            resize(EC_INIT_ENTITIES_SIZE);
        }
    };
}

#endif

