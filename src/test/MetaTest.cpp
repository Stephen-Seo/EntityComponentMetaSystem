
#include <gtest/gtest.h>

#include <EC/Meta/Meta.hpp>
#include <EC/EC.hpp>

struct C0 {};
struct C1 {};
struct C2 {};
struct C3 {};

struct T0 {};
struct T1 {};

using listAll = EC::Meta::TypeList<C0, C1, C2, C3>;
using listSome = EC::Meta::TypeList<C1, C3>;

using listTagsAll = EC::Meta::TypeList<T0, T1>;

TEST(Meta, Contains)
{
    int size = listAll::size;
    EXPECT_EQ(size, 4);

    bool result = EC::Meta::Contains<C0, listAll>::value;
    EXPECT_TRUE(result);
    result = EC::Meta::Contains<C1, listAll>::value;
    EXPECT_TRUE(result);
    result = EC::Meta::Contains<C2, listAll>::value;
    EXPECT_TRUE(result);
    result = EC::Meta::Contains<C3, listAll>::value;
    EXPECT_TRUE(result);

    size = listSome::size;
    EXPECT_EQ(size, 2);

    result = EC::Meta::Contains<C0, listSome>::value;
    EXPECT_FALSE(result);
    result = EC::Meta::Contains<C1, listSome>::value;
    EXPECT_TRUE(result);
    result = EC::Meta::Contains<C2, listSome>::value;
    EXPECT_FALSE(result);
    result = EC::Meta::Contains<C3, listSome>::value;
    EXPECT_TRUE(result);
}

TEST(Meta, ContainsAll)
{
    bool contains = EC::Meta::ContainsAll<listSome, listAll>::value;
    EXPECT_TRUE(contains);

    contains = EC::Meta::ContainsAll<listAll, listSome>::value;
    EXPECT_FALSE(contains);

    contains = EC::Meta::ContainsAll<listAll, listAll>::value;
    EXPECT_TRUE(contains);
}

TEST(Meta, IndexOf)
{
    int index = EC::Meta::IndexOf<C0, listAll>::value;
    EXPECT_EQ(index, 0);
    index = EC::Meta::IndexOf<C1, listAll>::value;
    EXPECT_EQ(index, 1);
    index = EC::Meta::IndexOf<C2, listAll>::value;
    EXPECT_EQ(index, 2);
    index = EC::Meta::IndexOf<C3, listAll>::value;
    EXPECT_EQ(index, 3);

    index = EC::Meta::IndexOf<C1, listSome>::value;
    EXPECT_EQ(index, 0);
    index = EC::Meta::IndexOf<C3, listSome>::value;
    EXPECT_EQ(index, 1);
}

TEST(Meta, Bitset)
{
    EC::Bitset<listAll, listTagsAll> bitset;
    EXPECT_EQ(bitset.size(), listAll::size + listTagsAll::size);

    bitset[EC::Meta::IndexOf<C1, listAll>::value] = true;
    EXPECT_TRUE(bitset.getComponentBit<C1>());
    bitset.flip();
    EXPECT_FALSE(bitset.getComponentBit<C1>());

    bitset.reset();
    bitset[listAll::size + EC::Meta::IndexOf<T0, listTagsAll>::value] = true;
    EXPECT_TRUE(bitset.getTagBit<T0>());
    bitset.flip();
    EXPECT_FALSE(bitset.getTagBit<T0>());
}

