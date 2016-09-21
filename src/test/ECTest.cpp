
#include <gtest/gtest.h>

#include <iostream>
#include <tuple>
#include <memory>

#include <EC/Meta/Meta.hpp>
#include <EC/EC.hpp>

struct C0 {
    C0(int x = 0, int y = 0) :
    x(x),
    y(y)
    {}

    int x, y;
};
struct C1 {
    int vx, vy;
};
struct C2 {};
struct C3 {};

struct T0 {};
struct T1 {};

using ListComponentsAll = EC::Meta::TypeList<C0, C1, C2, C3>;
using ListComponentsSome = EC::Meta::TypeList<C1, C3>;

using ListTagsAll = EC::Meta::TypeList<T0, T1>;

using ListAll = EC::Meta::TypeList<C0, C1, C2, C3, T0, T1>;

using EmptyList = EC::Meta::TypeList<>;

using MixedList = EC::Meta::TypeList<C2, T1>;

typedef std::unique_ptr<C0> C0Ptr;

struct Base
{
    virtual int getInt()
    {
        return 0;
    }
};

struct Derived : public Base
{
    virtual int getInt() override
    {
        return 1;
    }
};

typedef std::unique_ptr<Base> TestPtr;

TEST(EC, Bitset)
{
    {
        EC::Bitset<ListComponentsAll, EmptyList> bitset;
        bitset[1] = true;
        bitset[3] = true;

        auto genBitset = EC::Bitset<ListComponentsAll, EmptyList>::generateBitset<ListComponentsSome>();

        EXPECT_EQ(bitset, genBitset);
    }

    {
        EC::Bitset<ListAll, EmptyList> bitset;
        bitset[2] = true;
        bitset[5] = true;

        auto genBitset = EC::Bitset<ListAll, EmptyList>::generateBitset<MixedList>();

        EXPECT_EQ(bitset, genBitset);
    }
}

TEST(EC, Manager)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    std::size_t e0 = manager.addEntity();
    std::size_t e1 = manager.addEntity();

    manager.addComponent<C0>(e0, 5, 5);
    manager.addComponent<C1>(e0);

    manager.addComponent<C0>(e1);
    manager.addComponent<C1>(e1);
    manager.addTag<T0>(e1);

    {
        auto& vel = manager.getEntityData<C1>(e0);
        vel.vx = 1;
        vel.vy = 1;
    }

    auto posUpdate = [] (std::size_t id, C0& pos, C1& vel) {
        pos.x += vel.vx;
        pos.y += vel.vy;
    };

    auto updateTag = [] (std::size_t id, C0& pos, C1& vel) {
        pos.x = pos.y = vel.vx = vel.vy = 0;
    };

    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1> >(posUpdate);
    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1> >(posUpdate);

    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1, T0> >(updateTag);

    {
        auto& pos = manager.getEntityData<C0>(e0);
        EXPECT_EQ(pos.x, 7);
        EXPECT_EQ(pos.y, 7);
    }

    {
        bool has = manager.hasComponent<C0>(e1);

        EXPECT_TRUE(has);

        has = manager.hasTag<T0>(e1);

        EXPECT_TRUE(has);
    }

    manager.deleteEntity(e0);
    manager.cleanup();

    std::size_t edata = std::get<std::size_t>(manager.getEntityInfo(0));
    EXPECT_EQ(edata, 1);

    std::size_t e2 = manager.addEntity();

    manager.addTag<T0>(e2);

    std::size_t count = 0;

    auto updateTagOnly = [&count] (std::size_t id) {
        std::cout << "UpdateTagOnly was run." << std::endl;
        ++count;
    };

    manager.forMatchingSignature<EC::Meta::TypeList<T0> >(updateTagOnly);

    EXPECT_EQ(2, count);

    manager.deleteEntity(e1);
    manager.deleteEntity(e2);
    manager.cleanup();
}

TEST(EC, MoveComponentWithUniquePtr)
{
    {
        EC::Manager<EC::Meta::TypeList<C0Ptr>, EC::Meta::TypeList<> > manager;

        std::size_t e = manager.addEntity();

        {
            C0Ptr ptr = std::make_unique<C0>(5, 10);
            manager.addComponent<C0Ptr>(e, std::move(ptr));
        }

        int x = 0;
        int y = 0;
        manager.forMatchingSignature<EC::Meta::TypeList<C0Ptr> >([&x, &y] (std::size_t eID, C0Ptr& ptr) {
            x = ptr->x;
            y = ptr->y;
        });
        EXPECT_EQ(5, x);
        EXPECT_EQ(10, y);
    }
    {
        EC::Manager<EC::Meta::TypeList<TestPtr>, EC::Meta::TypeList<> > manager;

        std::size_t e = manager.addEntity();

        {
            TestPtr ptrBase = std::make_unique<Base>();
            manager.addComponent<TestPtr>(e, std::move(ptrBase));
        }

        int result = 0;

        auto getResultFunction = [&result] (std::size_t eID, TestPtr& ptr) {
            result = ptr->getInt();
        };

        manager.forMatchingSignature<EC::Meta::TypeList<TestPtr> >(getResultFunction);

        EXPECT_EQ(0, result);

        {
            TestPtr ptrDerived = std::make_unique<Derived>();
            manager.addComponent<TestPtr>(e, std::move(ptrDerived));
        }

        manager.forMatchingSignature<EC::Meta::TypeList<TestPtr> >(getResultFunction);

        EXPECT_EQ(1, result);
    }
}

TEST(EC, DeletedEntities)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    for(unsigned int i = 0; i < 4; ++i)
    {
        auto eid = manager.addEntity();
        manager.addComponent<C0>(eid);
        if(i >= 2)
        {
            manager.deleteEntity(eid);
        }
    }

    manager.cleanup();

    for(unsigned int i = 0; i < 4; ++i)
    {
        if(i < 2)
        {
            EXPECT_TRUE(manager.hasComponent<C0>(i));
        }
        else
        {
            EXPECT_FALSE(manager.hasComponent<C0>(i));
        }
    }

    for(unsigned int i = 0; i < 2; ++i)
    {
        auto eid = manager.addEntity();
        EXPECT_FALSE(manager.hasComponent<C0>(eid));
    }
}

TEST(EC, FunctionStorage)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;
    auto eid = manager.addEntity();
    manager.addComponent<C0>(eid, 0, 1);
    manager.addComponent<C1>(eid);
    manager.addComponent<C2>(eid);
    manager.addComponent<C3>(eid);

    auto f0index = manager.addForMatchingFunction<EC::Meta::TypeList<C0>>( [] (std::size_t eid, C0& c0) {
        ++c0.x;
        ++c0.y;
    });

    auto f1index = manager.addForMatchingFunction<EC::Meta::TypeList<C0, C1>>( [] (std::size_t eid, C0& c0, C1& c1) {
        c1.vx = c0.x + 10;
        c1.vy = c1.vy + c1.vx + c0.y + 10;
    });

    manager.addForMatchingFunction<EC::Meta::TypeList<>>(
        [] (std::size_t eid) {
            //derp 0
    });

    manager.addForMatchingFunction<EC::Meta::TypeList<>>(
        [] (std::size_t eid) {
            //derp 1
    });

    manager.callForMatchingFunctions();

    {
        auto c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0.x);
        EXPECT_EQ(2, c0.y);

        auto c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1.vx);
        EXPECT_EQ(23, c1.vy);
    }

    manager.clearSomeMatchingFunctions({f1index});

    {
        std::vector<unsigned long long> indices{f1index};
        manager.clearSomeMatchingFunctions(indices);
    }
    {
        std::set<unsigned long long> indices{f1index};
        manager.clearSomeMatchingFunctions(indices);
    }
    {
        std::unordered_set<unsigned long long> indices{f1index};
        manager.clearSomeMatchingFunctions(indices);
    }

    manager.callForMatchingFunctions();

    {
        auto c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0.x);
        EXPECT_EQ(2, c0.y);

        auto c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1.vx);
        EXPECT_EQ(46, c1.vy);
    }

    manager.clearForMatchingFunctions();
    manager.callForMatchingFunctions();

    {
        auto c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0.x);
        EXPECT_EQ(2, c0.y);

        auto c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1.vx);
        EXPECT_EQ(46, c1.vy);
    }
}

