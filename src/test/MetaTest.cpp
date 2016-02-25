
#include <gtest/gtest.h>

#include <EC/Meta.hpp>

TEST(Meta, Contains)
{
    struct C0 {};
    struct C1 {};
    struct C2 {};
    struct C3 {};

    using listAll = EC::Meta::TypeList<C0, C1, C2, C3>;

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

    using listSome = EC::Meta::TypeList<C1, C3>;
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
