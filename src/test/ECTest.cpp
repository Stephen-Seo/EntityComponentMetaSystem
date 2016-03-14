
#include <gtest/gtest.h>

#include <iostream>

#include <tuple>
#include <EC/Meta/Meta.hpp>
#include <EC/EC.hpp>

struct C0 {
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

    manager.addComponent<C0>(e0);
    manager.addComponent<C1>(e0);

    manager.addComponent<C0>(e1);
    manager.addComponent<C1>(e1);
    manager.addTag<T0>(e1);

    {
        auto& pos = manager.getEntityData<C0>(e0);
        pos.x = 5;
        pos.y = 5;
    }

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
}

