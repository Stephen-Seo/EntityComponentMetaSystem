#include "test_helpers.h"

#include <tuple>
#include <EC/Meta/Meta.hpp>
#include <EC/EC.hpp>

struct C0 {};
struct C1 {};
struct C2 {};
struct C3 {};

struct T0 {};
struct T1 {};

using ListComponentsAll = EC::Meta::TypeList<C0, C1, C2, C3>;
using ListComponentsSome = EC::Meta::TypeList<C1, C3>;

using ListTagsAll = EC::Meta::TypeList<T0, T1>;

using ListAll = EC::Meta::TypeList<C0, C1, C2, C3, T0, T1>;

using ListSome = EC::Meta::TypeList<C1, C3, T1>;

template <typename... STypes>
struct Storage
{
    using type = std::tuple<std::vector<STypes>... >;
};

void TEST_Meta_Contains()
{
    int size = ListComponentsAll::size;
    CHECK_EQ(size, 4);

    bool result = EC::Meta::Contains<C0, ListComponentsAll>::value;
    CHECK_TRUE(result);
    result = EC::Meta::Contains<C1, ListComponentsAll>::value;
    CHECK_TRUE(result);
    result = EC::Meta::Contains<C2, ListComponentsAll>::value;
    CHECK_TRUE(result);
    result = EC::Meta::Contains<C3, ListComponentsAll>::value;
    CHECK_TRUE(result);

    size = ListComponentsSome::size;
    CHECK_EQ(size, 2);

    result = EC::Meta::Contains<C0, ListComponentsSome>::value;
    CHECK_FALSE(result);
    result = EC::Meta::Contains<C1, ListComponentsSome>::value;
    CHECK_TRUE(result);
    result = EC::Meta::Contains<C2, ListComponentsSome>::value;
    CHECK_FALSE(result);
    result = EC::Meta::Contains<C3, ListComponentsSome>::value;
    CHECK_TRUE(result);
}

void TEST_Meta_ContainsAll()
{
    bool contains = EC::Meta::ContainsAll<ListComponentsSome, ListComponentsAll>::value;
    CHECK_TRUE(contains);

    contains = EC::Meta::ContainsAll<ListComponentsAll, ListComponentsSome>::value;
    CHECK_FALSE(contains);

    contains = EC::Meta::ContainsAll<ListComponentsAll, ListComponentsAll>::value;
    CHECK_TRUE(contains);
}

void TEST_Meta_IndexOf()
{
    int index = EC::Meta::IndexOf<C0, ListComponentsAll>::value;
    CHECK_EQ(index, 0);
    index = EC::Meta::IndexOf<C1, ListComponentsAll>::value;
    CHECK_EQ(index, 1);
    index = EC::Meta::IndexOf<C2, ListComponentsAll>::value;
    CHECK_EQ(index, 2);
    index = EC::Meta::IndexOf<C3, ListComponentsAll>::value;
    CHECK_EQ(index, 3);
    index = EC::Meta::IndexOf<T0, ListComponentsAll>::value;
    CHECK_EQ(index, 4);

    index = EC::Meta::IndexOf<C1, ListComponentsSome>::value;
    CHECK_EQ(index, 0);
    index = EC::Meta::IndexOf<C3, ListComponentsSome>::value;
    CHECK_EQ(index, 1);
    index = EC::Meta::IndexOf<C2, ListComponentsSome>::value;
    CHECK_EQ(index, 2);
}

void TEST_Meta_Bitset()
{
    EC::Bitset<ListComponentsAll, ListTagsAll> bitset;
    CHECK_EQ(bitset.size(), ListComponentsAll::size + ListTagsAll::size + 1);

    bitset[EC::Meta::IndexOf<C1, ListComponentsAll>::value] = true;
    CHECK_TRUE(bitset.getComponentBit<C1>());
    bitset.flip();
    CHECK_FALSE(bitset.getComponentBit<C1>());

    bitset.reset();
    bitset[ListComponentsAll::size + EC::Meta::IndexOf<T0, ListTagsAll>::value] = true;
    CHECK_TRUE(bitset.getTagBit<T0>());
    bitset.flip();
    CHECK_FALSE(bitset.getTagBit<T0>());
}

void TEST_Meta_Combine()
{
    using CombinedAll = EC::Meta::Combine<ListComponentsAll, ListTagsAll>;

    int listAllTemp = ListAll::size;
    int combinedAllTemp = CombinedAll::size;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<C0, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<C0, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<C1, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<C1, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<C2, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<C2, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<C3, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<C3, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<T0, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<T0, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    listAllTemp = EC::Meta::IndexOf<T1, ListAll>::value;
    combinedAllTemp = EC::Meta::IndexOf<T1, CombinedAll>::value;
    CHECK_EQ(combinedAllTemp, listAllTemp);

    bool same = std::is_same<CombinedAll, ListAll>::value;
    CHECK_TRUE(same);
}

void TEST_Meta_Morph()
{
    using TupleAll = std::tuple<C0, C1, C2, C3>;
    using MorphedTuple = EC::Meta::Morph<TupleAll, EC::Meta::TypeList<> >;

    int morphedTupleTemp = MorphedTuple::size;
    int componentsTemp = ListComponentsAll::size;
    CHECK_EQ(morphedTupleTemp, componentsTemp);

    morphedTupleTemp = EC::Meta::IndexOf<C0, MorphedTuple>::value;
    componentsTemp = EC::Meta::IndexOf<C0, ListComponentsAll>::value;
    CHECK_EQ(morphedTupleTemp, componentsTemp);

    morphedTupleTemp = EC::Meta::IndexOf<C1, MorphedTuple>::value;
    componentsTemp = EC::Meta::IndexOf<C1, ListComponentsAll>::value;
    CHECK_EQ(morphedTupleTemp, componentsTemp);

    morphedTupleTemp = EC::Meta::IndexOf<C2, MorphedTuple>::value;
    componentsTemp = EC::Meta::IndexOf<C2, ListComponentsAll>::value;
    CHECK_EQ(morphedTupleTemp, componentsTemp);

    morphedTupleTemp = EC::Meta::IndexOf<C3, MorphedTuple>::value;
    componentsTemp = EC::Meta::IndexOf<C3, ListComponentsAll>::value;
    CHECK_EQ(morphedTupleTemp, componentsTemp);

    using MorphedComponents = EC::Meta::Morph<ListComponentsAll, std::tuple<> >;
    bool isSame = std::is_same<MorphedComponents, TupleAll>::value;
    CHECK_TRUE(isSame);


    using ComponentsStorage = EC::Meta::Morph<ListComponentsAll, Storage<> >;

    isSame = std::is_same<ComponentsStorage::type,
        std::tuple<std::vector<C0>, std::vector<C1>, std::vector<C2>, std::vector<C3> > >::value;
    CHECK_TRUE(isSame);
}

void TEST_Meta_TypeListGet()
{
    bool isSame = std::is_same<C0, EC::Meta::TypeListGet<ListAll, 0> >::value;
    CHECK_TRUE(isSame);

    isSame = std::is_same<C1, EC::Meta::TypeListGet<ListAll, 1> >::value;
    CHECK_TRUE(isSame);

    isSame = std::is_same<C2, EC::Meta::TypeListGet<ListAll, 2> >::value;
    CHECK_TRUE(isSame);

    isSame = std::is_same<C3, EC::Meta::TypeListGet<ListAll, 3> >::value;
    CHECK_TRUE(isSame);

    const unsigned int temp = 4;
    isSame = std::is_same<T0, EC::Meta::TypeListGet<ListAll, temp> >::value;
    CHECK_TRUE(isSame);

    isSame = std::is_same<T1, EC::Meta::TypeListGet<ListAll, 5> >::value;
    CHECK_TRUE(isSame);
}

void TEST_Meta_ForEach()
{
    EC::Bitset<ListComponentsAll, ListTagsAll> bitset;

    auto setBits = [&bitset] (auto t) {
        bitset[EC::Meta::IndexOf<decltype(t), ListAll>::value] = true;
    };

    EC::Meta::forEach<ListComponentsSome>(setBits);

    CHECK_FALSE(bitset[0]);
    CHECK_TRUE(bitset[1]);
    CHECK_FALSE(bitset[2]);
    CHECK_TRUE(bitset[3]);
    CHECK_FALSE(bitset[4]);
    CHECK_FALSE(bitset[5]);
}

void TEST_Meta_Matching()
{
    {
        using Matched = EC::Meta::Matching<ListComponentsSome, ListComponentsAll>::type;
        bool isSame = std::is_same<ListComponentsSome, Matched>::value;
        CHECK_TRUE(isSame);
    }

    {
        using Matched = EC::Meta::Matching<ListSome, ListAll>::type;
        bool isSame = std::is_same<ListSome, Matched>::value;
        CHECK_TRUE(isSame);
    }

    {
        using Matched = EC::Meta::Matching<ListTagsAll, ListComponentsAll>::type;
        bool isSame = std::is_same<EC::Meta::TypeList<>, Matched>::value;
        CHECK_TRUE(isSame);
    }
}

