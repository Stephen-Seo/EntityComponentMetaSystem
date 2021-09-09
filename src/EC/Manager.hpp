
// This work derives from Vittorio Romeo's code used for cppcon 2015 licensed
// under the Academic Free License.
// His code is available here: https://github.com/SuperV1234/cppcon2015


#ifndef EC_MANAGER_HPP
#define EC_MANAGER_HPP

#define EC_INIT_ENTITIES_SIZE 256
#define EC_GROW_SIZE_AMOUNT 256

#include <array>
#include <cstddef>
#include <vector>
#include <deque>
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
#include <type_traits>

#ifndef NDEBUG
  #include <iostream>
#endif

#include "Meta/Combine.hpp"
#include "Meta/Matching.hpp"
#include "Meta/ForEachWithIndex.hpp"
#include "Meta/ForEachDoubleTuple.hpp"
#include "Meta/IndexOf.hpp"
#include "Bitset.hpp"

#include "ThreadPool.hpp"

namespace EC
{
    /*!
        \brief Manages an EntityComponent system.

        EC::Manager must be created with a list of all used Components and all
        used tags.

        Note that all components must have a default constructor.

        An optional third template parameter may be given, which is the size of
        the number of threads in the internal ThreadPool, and should be at
        least 2. If ThreadCount is 1 or less, then the ThreadPool will not be
        created and it will never be used, even if the "true" parameter is given
        for functions that enable its usage.

        Example:
        \code{.cpp}
            EC::Manager<TypeList<C0, C1, C2>, TypeList<T0, T1>> manager;
        \endcode
    */
    template <typename ComponentsList,
              typename TagsList,
              unsigned int ThreadCount = 4>
    struct Manager
    {
    public:
        using Components = ComponentsList;
        using Tags = TagsList;
        using Combined = EC::Meta::Combine<ComponentsList, TagsList>;
        using BitsetType = EC::Bitset<ComponentsList, TagsList>;

    private:
        using ComponentsTuple = EC::Meta::Morph<ComponentsList, std::tuple<> >;
        static_assert(std::is_default_constructible<ComponentsTuple>::value,
            "All components must be default constructible");

        template <typename... Types>
        struct Storage
        {
            using type = std::tuple<std::deque<Types>..., std::deque<char> >;
        };
        using ComponentsStorage =
            typename EC::Meta::Morph<ComponentsList, Storage<> >::type;

        // Entity: isAlive, ComponentsTags Info
        using EntitiesTupleType = std::tuple<bool, BitsetType>;
        using EntitiesType = std::deque<EntitiesTupleType>;

        EntitiesType entities;
        ComponentsStorage componentsStorage;
        std::size_t currentCapacity = 0;
        std::size_t currentSize = 0;
        std::unordered_set<std::size_t> deletedSet;

        std::unique_ptr<ThreadPool<ThreadCount> > threadPool;

    public:
        // section for "temporary" structures {{{
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructZero {
            std::array<std::size_t, 2> range;
            Manager *manager;
            EntitiesType *entities;
            const BitsetType *signature;
            void *userData;
        };
        /// Temporary struct used internally by ThreadPool
        template <typename Function>
        struct TPFnDataStructOne {
            std::array<std::size_t, 2> range;
            Manager *manager;
            EntitiesType *entities;
            BitsetType *signature;
            void *userData;
            Function *fn;
        };
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructTwo {
            std::array<std::size_t, 2> range;
            Manager *manager;
            EntitiesType *entities;
            void *userData;
            const std::vector<std::size_t> *matching;
        };
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructThree {
            std::array<std::size_t, 2> range;
            Manager *manager;
            std::vector<std::vector<std::size_t> > *matchingV;
            const std::vector<BitsetType*> *bitsets;
            EntitiesType *entities;
            std::mutex *mutex;
        };
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructFour {
            std::array<std::size_t, 2> range;
            Manager *manager;
            std::vector<std::vector<std::size_t> >*
                multiMatchingEntities;
            BitsetType *signatures;
            std::mutex *mutex;
        };
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructFive {
            std::array<std::size_t, 2> range;
            std::size_t index;
            Manager *manager;
            void *userData;
            std::vector<std::vector<std::size_t> >*
                multiMatchingEntities;
        };
        /// Temporary struct used internally by ThreadPool
        struct TPFnDataStructSix {
            std::array<std::size_t, 2> range;
            Manager *manager;
            std::vector<std::vector<std::size_t> > *
                multiMatchingEntities;
            BitsetType *bitsets;
            std::mutex *mutex;
        };
        /// Temporary struct used internally by ThreadPool
        template <typename Iterable>
        struct TPFnDataStructSeven {
            std::array<std::size_t, 2> range;
            Manager *manager;
            EntitiesType *entities;
            Iterable *iterable;
            void *userData;
        };
        // end section for "temporary" structures }}}

        /*!
            \brief Initializes the manager with a default capacity.

            The default capacity is set with macro EC_INIT_ENTITIES_SIZE,
            and will grow by amounts of EC_GROW_SIZE_AMOUNT when needed.
        */
        Manager()
        {
            resize(EC_INIT_ENTITIES_SIZE);
            if(ThreadCount >= 2) {
                threadPool = std::make_unique<ThreadPool<ThreadCount> >();
            }
        }

    private:
        void resize(std::size_t newCapacity)
        {
            if(currentCapacity >= newCapacity)
            {
                return;
            }

            EC::Meta::forEach<ComponentsList>([this, newCapacity] (auto t) {
                std::get<std::deque<decltype(t)> >(
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
            \brief Returns a pointer to a component belonging to the given
                Entity.

            This function will return a pointer to a Component regardless of
            whether or not the Entity actually owns the Component. If the Entity
            doesn't own the Component, changes to the Component will not affect
            any Entity. It is recommended to use hasComponent() to determine if
            the Entity actually owns that Component.

            If the given Component is unknown to the Manager, then this function
            will return a nullptr.
        */
        template <typename Component>
        Component* getEntityData(const std::size_t& index)
        {
            constexpr auto componentIndex = EC::Meta::IndexOf<
                Component, Components>::value;
            if(componentIndex < Components::size)
            {
                // Cast required due to compiler thinking that an invalid
                // Component is needed even though the enclosing if statement
                // prevents this from ever happening.
                return (Component*) &std::get<componentIndex>(
                    componentsStorage).at(index);
            }
            else
            {
                return nullptr;
            }
        }

        /*!
            \brief Returns a pointer to a component belonging to the given
                Entity.

            Note that this function is the same as getEntityData().

            This function will return a pointer to a Component regardless of
            whether or not the Entity actually owns the Component. If the Entity
            doesn't own the Component, changes to the Component will not affect
            any Entity. It is recommended to use hasComponent() to determine if
            the Entity actually owns that Component.

            If the given Component is unknown to the Manager, then this function
            will return a nullptr.
        */
        template <typename Component>
        Component* getEntityComponent(const std::size_t& index)
        {
            return getEntityData<Component>(index);
        }

        /*!
            \brief Returns a const pointer to a component belonging to the
                given Entity.

            This function will return a const pointer to a Component
            regardless of whether or not the Entity actually owns the Component.
            If the Entity doesn't own the Component, changes to the Component
            will not affect any Entity. It is recommended to use hasComponent()
            to determine if the Entity actually owns that Component.

            If the given Component is unknown to the Manager, then this function
            will return a nullptr.
        */
        template <typename Component>
        const Component* getEntityData(const std::size_t& index) const
        {
            constexpr auto componentIndex = EC::Meta::IndexOf<
                Component, Components>::value;
            if(componentIndex < Components::size)
            {
                // Cast required due to compiler thinking that an invalid
                // Component is needed even though the enclosing if statement
                // prevents this from ever happening.
                return (Component*) &std::get<componentIndex>(
                    componentsStorage).at(index);
            }
            else
            {
                return nullptr;
            }
        }

        /*!
            \brief Returns a const pointer to a component belonging to the
                given Entity.

            Note that this function is the same as getEntityData() (const).

            This function will return a const pointer to a Component
            regardless of whether or not the Entity actually owns the Component.
            If the Entity doesn't own the Component, changes to the Component
            will not affect any Entity. It is recommended to use hasComponent()
            to determine if the Entity actually owns that Component.

            If the given Component is unknown to the Manager, then this function
            will return a nullptr.
        */
        template <typename Component>
        const Component* getEntityComponent(const std::size_t& index) const
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

            If the Entity is not alive or the given Component is not known to
            the Manager, then nothing will change.

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
            if(!EC::Meta::Contains<Component, Components>::value
                || !isAlive(entityID))
            {
                return;
            }

            Component component(std::forward<Args>(args)...);

            std::get<BitsetType>(
                entities[entityID]
            ).template getComponentBit<Component>() = true;

            constexpr auto index =
                EC::Meta::IndexOf<Component, Components>::value;

            // Cast required due to compiler thinking that deque<char> at
            // index = Components::size is being used, even if the previous
            // if statement will prevent this from ever happening.
            (*((std::deque<Component>*)(&std::get<index>(
                componentsStorage
            ))))[entityID] = std::move(component);
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
            if(!EC::Meta::Contains<Component, Components>::value
                || !isAlive(entityID))
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
            if(!EC::Meta::Contains<Tag, Tags>::value
                || !isAlive(entityID))
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
            if(!EC::Meta::Contains<Tag, Tags>::value
                || !isAlive(entityID))
            {
                return;
            }

            std::get<BitsetType>(
                entities[entityID]
            ).template getTagBit<Tag>() = false;
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

    private:
        template <typename... Types>
        struct ForMatchingSignatureHelper
        {
            template <typename CType, typename Function>
            static void call(
                const std::size_t& entityID,
                CType& ctype,
                Function&& function,
                void* userData = nullptr)
            {
                function(
                    entityID,
                    userData,
                    ctype.template getEntityData<Types>(entityID)...
                );
            }

            template <typename CType, typename Function>
            static void callPtr(
                const std::size_t& entityID,
                CType& ctype,
                Function* function,
                void* userData = nullptr)
            {
                (*function)(
                    entityID,
                    userData,
                    ctype.template getEntityData<Types>(entityID)...
                );
            }

            template <typename CType, typename Function>
            void callInstance(
                const std::size_t& entityID,
                CType& ctype,
                Function&& function,
                void* userData = nullptr) const
            {
                ForMatchingSignatureHelper<Types...>::call(
                    entityID,
                    ctype,
                    std::forward<Function>(function),
                    userData);
            }

            template <typename CType, typename Function>
            void callInstancePtr(
                const std::size_t& entityID,
                CType& ctype,
                Function* function,
                void* userData = nullptr) const
            {
                ForMatchingSignatureHelper<Types...>::callPtr(
                    entityID,
                    ctype,
                    function,
                    userData);
            }
        };

    public:
        /*!
            \brief Calls the given function on all Entities matching the given
                Signature.

            The function object given to this function must accept std::size_t
            as its first parameter, void* as its second parameter, and Component
            pointers for the rest of the parameters. Tags specified in the
            Signature are only used as filters and will not be given as a
            parameter to the function.

            The second parameter is default nullptr and will be passed to the
            function call as the second parameter as a means of providing
            context (useful when the function is not a lambda function).

            The third parameter is default false (not multi-threaded).
            Otherwise, if true, then the thread pool will be used to call the
            given function in parallel across all entities. Note that
            multi-threading is based on splitting the task of calling the
            function across sections of entities. Thus if there are only a small
            amount of entities in the manager, then using multiple threads may
            not have as great of a speed-up.

            Example:
            \code{.cpp}
                Context c; // some class/struct with data
                manager.forMatchingSignature<TypeList<C0, C1, T0>>([]
                    (std::size_t ID,
                    void* context,
                    C0* component0, C1* component1)
                {
                    // Lambda function contents here
                },
                &c, // "Context" object passed to the function
                true // enable use of internal ThreadPool
                );
            \endcode
            Note, the ID given to the function is not permanent. An entity's ID
            may change when cleanup() is called.
        */
        template <typename Signature, typename Function>
        void forMatchingSignature(Function&& function,
            void* userData = nullptr,
            const bool useThreadPool = false)
        {
            using SignatureComponents =
                typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper =
                EC::Meta::Morph<
                    SignatureComponents,
                    ForMatchingSignatureHelper<> >;

            BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();
            if(!useThreadPool || !threadPool)
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
                            std::forward<Function>(function), userData);
                    }
                }
            }
            else
            {
                std::array<TPFnDataStructZero, ThreadCount * 2> fnDataAr;

                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].entities = &entities;
                    fnDataAr[i].signature = &signatureBitset;
                    fnDataAr[i].userData = userData;

                    threadPool->queueFn([&function] (void *ud) {
                        auto *data = static_cast<TPFnDataStructZero*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }

                            if(((*data->signature)
                                        & std::get<BitsetType>(
                                            data->entities->at(i)))
                                    == *data->signature) {
                                Helper::call(i,
                                             *data->manager,
                                             std::forward<Function>(function),
                                             data->userData);
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }
        }

        /*!
            \brief Calls the given function on all Entities matching the given
                Signature.

            The function pointer given to this function must accept std::size_t
            as its first parameter, void* as its second parameter,  and
            Component pointers for the rest of the parameters. Tags specified in
            the Signature are only used as filters and will not be given as a
            parameter to the function.

            The second parameter is default nullptr and will be passed to the
            function call as the second parameter as a means of providing
            context (useful when the function is not a lambda function).

            The third parameter is default false (not multi-threaded).
            Otherwise, if true, then the thread pool will be used to call the
            given function in parallel across all entities. Note that
            multi-threading is based on splitting the task of calling the
            function across sections of entities. Thus if there are only a small
            amount of entities in the manager, then using multiple threads may
            not have as great of a speed-up.

            Example:
            \code{.cpp}
                Context c; // some class/struct with data
                auto function = []
                    (std::size_t ID,
                    void* context,
                    C0* component0, C1* component1)
                {
                    // Lambda function contents here
                };
                manager.forMatchingSignaturePtr<TypeList<C0, C1, T0>>(
                    &function, // ptr
                    &c, // "Context" object passed to the function
                    true // enable use of ThreadPool
                );
            \endcode
            Note, the ID given to the function is not permanent. An entity's ID
            may change when cleanup() is called.
        */
        template <typename Signature, typename Function>
        void forMatchingSignaturePtr(Function* function,
            void* userData = nullptr,
            const bool useThreadPool = false)
        {
            using SignatureComponents =
                typename EC::Meta::Matching<Signature, ComponentsList>::type;
            using Helper =
                EC::Meta::Morph<
                    SignatureComponents,
                    ForMatchingSignatureHelper<> >;

            BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();
            if(!useThreadPool || !threadPool)
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
                        Helper::callPtr(i, *this, function, userData);
                    }
                }
            }
            else
            {
                std::array<TPFnDataStructOne<Function>, ThreadCount * 2> fnDataAr;

                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].entities = &entities;
                    fnDataAr[i].signature = &signatureBitset;
                    fnDataAr[i].userData = userData;
                    fnDataAr[i].fn = function;
                    threadPool->queueFn([] (void *ud) {
                        auto *data = static_cast<TPFnDataStructOne<Function>*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }

                            if(((*data->signature)
                                        & std::get<BitsetType>(
                                            data->entities->at(i)))
                                    == *data->signature) {
                                Helper::callPtr(i,
                                                *data->manager,
                                                data->fn,
                                                data->userData);
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }
        }



    private:
        std::map<std::size_t, std::tuple<
            BitsetType,
            void*,
            std::function<void(
                std::size_t,
                std::vector<std::size_t>,
                void*)> > >
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

            Note that the context pointer provided here (default nullptr) will
            be provided to the stored function when called.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([]
                    (std::size_t ID,
                    void* context,
                    C0* component0, C1* component1)
                {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode

            \return The index of the function, used for deletion with
                removeForMatchingFunction() or filtering with
                keepSomeMatchingFunctions() or removeSomeMatchingFunctions(),
                or calling with callForMatchingFunction().
        */
        template <typename Signature, typename Function>
        std::size_t addForMatchingFunction(
            Function&& function,
            void* userData = nullptr)
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
                    userData,
                    [function, helper, this]
                        (const bool useThreadPool,
                        std::vector<std::size_t> matching,
                        void* userData)
                {
                    if(!useThreadPool || !threadPool)
                    {
                        for(auto eid : matching)
                        {
                            if(isAlive(eid))
                            {
                                helper.callInstancePtr(
                                    eid, *this, &function, userData);
                            }
                        }
                    }
                    else
                    {
                        std::array<TPFnDataStructTwo, ThreadCount * 2> fnDataAr;

                        std::size_t s = matching.size() / (ThreadCount * 2);
                        for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                            std::size_t begin = s * i;
                            std::size_t end;
                            if(i == ThreadCount * 2 - 1) {
                                end = matching.size();
                            } else {
                                end = s * (i + 1);
                            }
                            if(begin == end) {
                                continue;
                            }
                            fnDataAr[i].range = {begin, end};
                            fnDataAr[i].manager = this;
                            fnDataAr[i].entities = &entities;
                            fnDataAr[i].userData = userData;
                            fnDataAr[i].matching = &matching;
                            threadPool->queueFn([&function, helper] (void* ud) {
                                auto *data = static_cast<TPFnDataStructTwo*>(ud);
                                for(std::size_t i = data->range[0];
                                        i < data->range[1];
                                        ++i) {
                                    if(data->manager->isAlive(
                                            data->matching->at(i))) {
                                        helper.callInstancePtr(
                                            data->matching->at(i),
                                            *data->manager,
                                            &function,
                                            data->userData);
                                    }
                                }
                            }, &fnDataAr[i]);
                        }
                        threadPool->easyWakeAndWait();
                    }
                })));

            return functionIndex++;
        }

    private:
        std::vector<std::vector<std::size_t> > getMatchingEntities(
            std::vector<BitsetType*> bitsets, const bool useThreadPool = false)
        {
            std::vector<std::vector<std::size_t> > matchingV(bitsets.size());

            if(!useThreadPool || !threadPool)
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
                std::array<TPFnDataStructThree, ThreadCount * 2> fnDataAr;

                std::size_t s = currentSize / (ThreadCount * 2);
                std::mutex mutex;
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].matchingV = &matchingV;
                    fnDataAr[i].bitsets = &bitsets;
                    fnDataAr[i].entities = &entities;
                    fnDataAr[i].mutex = &mutex;
                    threadPool->queueFn([] (void *ud) {
                        auto *data = static_cast<TPFnDataStructThree*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }
                            for(std::size_t j = 0; j < data->bitsets->size();
                                    ++j) {
                                if((*data->bitsets->at(j)
                                            & std::get<BitsetType>(
                                                data->entities->at(i)))
                                        == *data->bitsets->at(j)) {
                                    std::lock_guard<std::mutex> lock(
                                            *data->mutex);
                                    data->matchingV->at(j).push_back(i);
                                }
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }

            return matchingV;
        }

    public:

        /*!
            \brief Call all stored functions.

            The first (and only) parameter can be optionally used to enable the
            use of the internal ThreadPool to call all stored functions in
            parallel. Using the value false (which is the default) will not use
            the ThreadPool and run all stored functions sequentially on the main
            thread.  Note that multi-threading is based on splitting the task of
            calling the functions across sections of entities. Thus if there are
            only a small amount of entities in the manager, then using multiple
            threads may not have as great of a speed-up.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([]
                    (std::size_t ID,
                    void* context,
                    C0* component0, C1* component1) {
                    // Lambda function contents here
                });

                // call all stored functions
                manager.callForMatchingFunctions();

                // call all stored functions with ThreadPool enabled
                manager.callForMatchingFunctions(true);

                // remove all stored functions
                manager.clearForMatchingFunctions();
            \endcode
        */
        void callForMatchingFunctions(const bool useThreadPool = false)
        {
            std::vector<BitsetType*> bitsets;
            for(auto iter = forMatchingFunctions.begin();
                iter != forMatchingFunctions.end();
                ++iter)
            {
                bitsets.push_back(&std::get<BitsetType>(iter->second));
            }

            std::vector<std::vector<std::size_t> > matching =
                getMatchingEntities(bitsets, useThreadPool);

            std::size_t i = 0;
            for(auto iter = forMatchingFunctions.begin();
                iter != forMatchingFunctions.end();
                ++iter)
            {
                std::get<2>(iter->second)(
                    useThreadPool, matching[i++], std::get<1>(iter->second));
            }
        }

        /*!
            \brief Call a specific stored function.

            The second parameter can be optionally used to enable the use of the
            internal ThreadPool to call the stored function in parallel. Using
            the value false (which is the default) will not use the ThreadPool
            and run the stored function sequentially on the main thread.  Note
            that multi-threading is based on splitting the task of calling the
            functions across sections of entities. Thus if there are only a
            small amount of entities in the manager, then using multiple threads
            may not have as great of a speed-up.

            Example:
            \code{.cpp}
                std::size_t id =
                    manager.addForMatchingFunction<TypeList<C0, C1, T0>>(
                        [] (std::size_t ID, void* context, C0* c0, C1* c1) {
                    // Lambda function contents here
                });

                // call the previously added function
                manager.callForMatchingFunction(id);

                // call the previously added function with ThreadPool enabled
                manager.callForMatchingFunction(id, true);
            \endcode

            \return False if a function with the given id does not exist.
        */
        bool callForMatchingFunction(std::size_t id,
                                     const bool useThreadPool = false)
        {
            auto iter = forMatchingFunctions.find(id);
            if(iter == forMatchingFunctions.end())
            {
                return false;
            }
            std::vector<std::vector<std::size_t> > matching =
                getMatchingEntities(std::vector<BitsetType*>{
                    &std::get<BitsetType>(iter->second)}, useThreadPool);
            std::get<2>(iter->second)(
                useThreadPool, matching[0], std::get<1>(iter->second));
            return true;
        }

        /*!
            \brief Remove all stored functions.

            Also resets the index counter of stored functions to 0.

            Example:
            \code{.cpp}
                manager.addForMatchingFunction<TypeList<C0, C1, T0>>([]
                    (std::size_t ID,
                    void* context,
                    C0* component0, C1* component1)
                {
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
            \brief Sets the context pointer of a stored function

            \return True if id is valid and context was updated
        */
        bool changeForMatchingFunctionContext(std::size_t id, void* userData)
        {
            auto f = forMatchingFunctions.find(id);
            if(f != forMatchingFunctions.end())
            {
                std::get<1>(f->second) = userData;
                return true;
            }
            return false;
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

            The second parameter (default nullptr) will be provided to every
            function call as a void* (context).

            The third parameter is default false (not multi-threaded).
            Otherwise, if true, then the thread pool will be used to call the
            given function in parallel across all entities. Note that
            multi-threading is based on splitting the task of calling the
            function across sections of entities. Thus if there are only a small
            amount of entities in the manager, then using multiple threads may
            not have as great of a speed-up.

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
            FTuple fTuple,
            void* userData = nullptr,
            const bool useThreadPool = false)
        {
            std::vector<std::vector<std::size_t> >
                multiMatchingEntities(SigList::size);
            BitsetType signatureBitsets[SigList::size];

            // generate bitsets for each signature
            EC::Meta::forEachWithIndex<SigList>(
            [&signatureBitsets] (auto signature, const auto index) {
                signatureBitsets[index] =
                    BitsetType::template generateBitset
                        <decltype(signature)>();
            });

            // find and store entities matching signatures
            if(!useThreadPool || !threadPool)
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
                std::array<TPFnDataStructFour, ThreadCount * 2> fnDataAr;

                std::mutex mutex;
                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].multiMatchingEntities = &multiMatchingEntities;
                    fnDataAr[i].signatures = signatureBitsets;
                    fnDataAr[i].mutex = &mutex;

                    threadPool->queueFn([] (void *ud) {
                        auto *data = static_cast<TPFnDataStructFour*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }
                            for(std::size_t j = 0; j < SigList::size; ++j) {
                                if((data->signatures[j]
                                                & std::get<BitsetType>(
                                                    data->manager->entities[i]))
                                        == data->signatures[j]) {
                                    std::lock_guard<std::mutex> lock(
                                        *data->mutex);
                                    data->multiMatchingEntities->at(j)
                                        .push_back(i);
                                }
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }

            // call functions on matching entities
            EC::Meta::forEachDoubleTuple(
                EC::Meta::Morph<SigList, std::tuple<> >{},
                fTuple,
                [this, &multiMatchingEntities, useThreadPool, &userData]
                (auto sig, auto func, auto index)
                {
                    using SignatureComponents =
                        typename EC::Meta::Matching<
                            decltype(sig), ComponentsList>::type;
                    using Helper =
                        EC::Meta::Morph<
                            SignatureComponents,
                            ForMatchingSignatureHelper<> >;
                    if(!useThreadPool || !threadPool) {
                        for(const auto& id : multiMatchingEntities[index]) {
                            if(isAlive(id)) {
                                Helper::call(id, *this, func, userData);
                            }
                        }
                    } else {
                        std::array<TPFnDataStructFive, ThreadCount * 2>
                            fnDataAr;
                        std::size_t s = multiMatchingEntities[index].size()
                            / (ThreadCount * 2);
                        for(unsigned int i = 0; i < ThreadCount * 2; ++i) {
                            std::size_t begin = s * i;
                            std::size_t end;
                            if(i == ThreadCount * 2 - 1) {
                                end = multiMatchingEntities[index].size();
                            } else {
                                end = s * (i + 1);
                            }
                            if(begin == end) {
                                continue;
                            }
                            fnDataAr[i].range = {begin, end};
                            fnDataAr[i].index = index;
                            fnDataAr[i].manager = this;
                            fnDataAr[i].userData = userData;
                            fnDataAr[i].multiMatchingEntities =
                                &multiMatchingEntities;
                            threadPool->queueFn([&func] (void *ud) {
                                auto *data = static_cast<TPFnDataStructFive*>(ud);
                                for(std::size_t i = data->range[0];
                                        i < data->range[1]; ++i) {
                                    if(data->manager->isAlive(
                                            data->multiMatchingEntities
                                                ->at(data->index).at(i))) {
                                        Helper::call(
                                            data->multiMatchingEntities
                                                ->at(data->index).at(i),
                                            *data->manager,
                                            func,
                                            data->userData);
                                    }
                                }
                            }, &fnDataAr[i]);
                        }
                        threadPool->easyWakeAndWait();
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

            The second parameter (default nullptr) will be provided to every
            function call as a void* (context).

            The third parameter is default false (not multi-threaded).
            Otherwise, if true, then the thread pool will be used to call the
            given function in parallel across all entities. Note that
            multi-threading is based on splitting the task of calling the
            function across sections of entities. Thus if there are only a small
            amount of entities in the manager, then using multiple threads may
            not have as great of a speed-up.

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
            void* userData = nullptr,
            const bool useThreadPool = false)
        {
            std::vector<std::vector<std::size_t> > multiMatchingEntities(
                SigList::size);
            BitsetType signatureBitsets[SigList::size];

            // generate bitsets for each signature
            EC::Meta::forEachWithIndex<SigList>(
            [&signatureBitsets] (auto signature, const auto index) {
                signatureBitsets[index] =
                    BitsetType::template generateBitset
                        <decltype(signature)>();
            });

            // find and store entities matching signatures
            if(!useThreadPool || !threadPool)
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
                std::array<TPFnDataStructSix, ThreadCount * 2> fnDataAr;

                std::mutex mutex;
                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].multiMatchingEntities = &multiMatchingEntities;
                    fnDataAr[i].bitsets = signatureBitsets;
                    fnDataAr[i].mutex = &mutex;

                    threadPool->queueFn([] (void *ud) {
                        auto *data = static_cast<TPFnDataStructSix*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }
                            for(std::size_t j = 0; j < SigList::size; ++j) {
                                if((data->bitsets[j]
                                            & std::get<BitsetType>(
                                                data->manager->entities[i]))
                                        == data->bitsets[j]) {
                                    std::lock_guard<std::mutex> lock(
                                        *data->mutex);
                                    data->multiMatchingEntities->at(j)
                                        .push_back(i);
                                }
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }

            // call functions on matching entities
            EC::Meta::forEachDoubleTuple(
                EC::Meta::Morph<SigList, std::tuple<> >{},
                fTuple,
                [this, &multiMatchingEntities, useThreadPool, &userData]
                (auto sig, auto func, auto index)
                {
                    using SignatureComponents =
                        typename EC::Meta::Matching<
                            decltype(sig), ComponentsList>::type;
                    using Helper =
                        EC::Meta::Morph<
                            SignatureComponents,
                            ForMatchingSignatureHelper<> >;
                    if(!useThreadPool || !threadPool)
                    {
                        for(const auto& id : multiMatchingEntities[index])
                        {
                            if(isAlive(id))
                            {
                                Helper::callPtr(id, *this, func, userData);
                            }
                        }
                    }
                    else
                    {
                        std::array<TPFnDataStructFive, ThreadCount * 2>
                            fnDataAr;
                        std::size_t s = multiMatchingEntities[index].size()
                            / (ThreadCount * 2);
                        for(unsigned int i = 0; i < ThreadCount * 2; ++i) {
                            std::size_t begin = s * i;
                            std::size_t end;
                            if(i == ThreadCount * 2 - 1) {
                                end = multiMatchingEntities[index].size();
                            } else {
                                end = s * (i + 1);
                            }
                            if(begin == end) {
                                continue;
                            }
                            fnDataAr[i].range = {begin, end};
                            fnDataAr[i].index = index;
                            fnDataAr[i].manager = this;
                            fnDataAr[i].userData = userData;
                            fnDataAr[i].multiMatchingEntities =
                                &multiMatchingEntities;
                            threadPool->queueFn([&func] (void *ud) {
                                auto *data = static_cast<TPFnDataStructFive*>(ud);
                                for(std::size_t i = data->range[0];
                                        i < data->range[1]; ++i) {
                                    if(data->manager->isAlive(
                                            data->multiMatchingEntities
                                                ->at(data->index).at(i))) {
                                        Helper::callPtr(
                                            data->multiMatchingEntities
                                                ->at(data->index).at(i),
                                            *data->manager,
                                            func,
                                            data->userData);
                                    }
                                }
                            }, &fnDataAr[i]);
                        }
                        threadPool->easyWakeAndWait();
                    }
                }
            );
        }

        typedef void ForMatchingFn(std::size_t,
                                   Manager<ComponentsList, TagsList>*,
                                   void*);

        /*!
            \brief A simple version of forMatchingSignature()

            This function behaves like forMatchingSignature(), but instead of
            providing a function with each requested component as a parameter,
            the function receives a pointer to the manager itself, with which to
            query component/tag data.

            The third parameter can be optionally used to enable the use of the
            internal ThreadPool to call the function in parallel. Using the
            value false (which is the default) will not use the ThreadPool and
            run the function sequentially on all entities on the main thread.
            Note that multi-threading is based on splitting the task of calling
            the functions across sections of entities. Thus if there are only a
            small amount of entities in the manager, then using multiple threads
            may not have as great of a speed-up.
         */
        template <typename Signature>
        void forMatchingSimple(ForMatchingFn fn, 
                               void *userData = nullptr,
                               const bool useThreadPool = false) {
            const BitsetType signatureBitset =
                BitsetType::template generateBitset<Signature>();
            if(!useThreadPool || !threadPool) {
                for(std::size_t i = 0; i < currentSize; ++i) {
                    if(!std::get<bool>(entities[i])) {
                        continue;
                    } else if((signatureBitset
                                & std::get<BitsetType>(entities[i]))
                            == signatureBitset) {
                        fn(i, this, userData);
                    }
                }
            } else {
                std::array<TPFnDataStructZero, ThreadCount * 2> fnDataAr;

                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].entities = &entities;
                    fnDataAr[i].signature = &signatureBitset;
                    fnDataAr[i].userData = userData;
                    threadPool->queueFn([&fn] (void *ud) {
                        auto *data = static_cast<TPFnDataStructZero*>(ud);
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            } else if((*data->signature 
                                        & std::get<BitsetType>(
                                            data->entities->at(i)))
                                    == *data->signature) {
                                fn(i, data->manager, data->userData);
                            }
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }
        }

        /*!
            \brief Similar to forMatchingSimple(), but with a collection of Component/Tag indices

            This function works like forMatchingSimple(), but instead of
            providing template types that filter out non-matching entities, an
            iterable of indices must be provided which correlate to matching
            Component/Tag indices. The function given must match the previously
            defined typedef of type ForMatchingFn.

            The fourth parameter can be optionally used to enable the use of the
            internal ThreadPool to call the function in parallel. Using the
            value false (which is the default) will not use the ThreadPool and
            run the function sequentially on all entities on the main thread.
            Note that multi-threading is based on splitting the task of calling
            the functions across sections of entities. Thus if there are only a
            small amount of entities in the manager, then using multiple threads
            may not have as great of a speed-up.
         */
        template <typename Iterable>
        void forMatchingIterable(Iterable iterable, 
                                 ForMatchingFn fn,
                                 void* userData = nullptr,
                                 const bool useThreadPool = false) {
            if(!useThreadPool || !threadPool) {
                bool isValid;
                for(std::size_t i = 0; i < currentSize; ++i) {
                    if(!std::get<bool>(entities[i])) {
                        continue;
                    }

                    isValid = true;
                    for(const auto& integralValue : iterable) {
                        if(!std::get<BitsetType>(entities[i]).getCombinedBit(
                                integralValue)) {
                            isValid = false;
                            break;
                        }
                    }
                    if(!isValid) { continue; }

                    fn(i, this, userData);
                }
            } else {
                std::array<TPFnDataStructSeven<Iterable>, ThreadCount * 2>
                    fnDataAr;

                std::size_t s = currentSize / (ThreadCount * 2);
                for(std::size_t i = 0; i < ThreadCount * 2; ++i) {
                    std::size_t begin = s * i;
                    std::size_t end;
                    if(i == ThreadCount * 2 - 1) {
                        end = currentSize;
                    } else {
                        end = s * (i + 1);
                    }
                    if(begin == end) {
                        continue;
                    }
                    fnDataAr[i].range = {begin, end};
                    fnDataAr[i].manager = this;
                    fnDataAr[i].entities = &entities;
                    fnDataAr[i].iterable = &iterable;
                    fnDataAr[i].userData = userData;
                    threadPool->queueFn([&fn] (void *ud) {
                        auto *data = static_cast<TPFnDataStructSeven<Iterable>*>(ud);
                        bool isValid;
                        for(std::size_t i = data->range[0]; i < data->range[1];
                                ++i) {
                            if(!data->manager->isAlive(i)) {
                                continue;
                            }
                            isValid = true;
                            for(const auto& integralValue : *data->iterable) {
                                if(!std::get<BitsetType>(data->entities->at(i))
                                        .getCombinedBit(integralValue)) {
                                    isValid = false;
                                    break;
                                }
                            }
                            if(!isValid) { continue; }

                            fn(i, data->manager, data->userData);
                        }
                    }, &fnDataAr[i]);
                }
                threadPool->easyWakeAndWait();
            }
        }
    };
}

#endif

