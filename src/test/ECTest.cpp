
#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

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

using ListCombinedComponentsTags = EC::Meta::Combine<ListComponentsAll, ListTagsAll>;

typedef std::unique_ptr<C0> C0Ptr;

struct Base
{
    virtual int getInt()
    {
        return 0;
    }

    virtual ~Base() {}
};

struct Derived : public Base
{
    virtual int getInt() override
    {
        return 1;
    }
};

typedef std::unique_ptr<Base> TestPtr;

struct Context
{
    int a, b;
};

void assignContextToC0(std::size_t /* id */, void* context, C0* c)
{
    Context* contextPtr = (Context*) context;
    c->x = contextPtr->a;
    c->y = contextPtr->b;
}

void assignC0ToContext(std::size_t /* id */, void* context, C0* c)
{
    Context* contextPtr = (Context*) context;
    contextPtr->a = c->x;
    contextPtr->b = c->y;
}

void setC0ToOneAndTwo(std::size_t /* id */, void* /* context */, C0* c)
{
    c->x = 1;
    c->y = 2;
}

void setContextToThreeAndFour(std::size_t /* id */, void* context, C0* /* c */)
{
    Context* contextPtr = (Context*) context;
    contextPtr->a = 3;
    contextPtr->b = 4;
}

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
        auto* vel = manager.getEntityData<C1>(e0);
        vel->vx = 1;
        vel->vy = 1;
    }

    auto posUpdate = []
        (std::size_t /* id */, void* /* context */, C0* pos, C1* vel)
    {
        pos->x += vel->vx;
        pos->y += vel->vy;
    };

    auto updateTag = []
        (std::size_t /* id */, void* /* context */, C0* pos, C1* vel)
    {
        pos->x = pos->y = vel->vx = vel->vy = 0;
    };

    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1> >(posUpdate);
    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1> >(posUpdate);

    manager.forMatchingSignature<EC::Meta::TypeList<C0, C1, T0> >(updateTag);

    {
        auto* pos = manager.getEntityData<C0>(e0);
        EXPECT_EQ(pos->x, 7);
        EXPECT_EQ(pos->y, 7);
    }

    {
        bool has = manager.hasComponent<C0>(e1);

        EXPECT_TRUE(has);

        has = manager.hasTag<T0>(e1);

        EXPECT_TRUE(has);
    }

    manager.deleteEntity(e0);

    std::size_t e2 = manager.addEntity();

    manager.addTag<T0>(e2);

    std::size_t count = 0;

    auto updateTagOnly = [&count] (std::size_t /* id */, void* /* context */) {
        std::cout << "UpdateTagOnly was run." << std::endl;
        ++count;
    };

    manager.forMatchingSignature<EC::Meta::TypeList<T0> >(updateTagOnly);

    EXPECT_EQ(2, count);

    manager.deleteEntity(e1);
    manager.deleteEntity(e2);
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
        manager.forMatchingSignature<EC::Meta::TypeList<C0Ptr> >([&x, &y]
                (std::size_t /* id */, void* /* context */, C0Ptr* ptr) {
            x = (*ptr)->x;
            y = (*ptr)->y;
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

        auto getResultFunction = [&result]
            (std::size_t /* id */, void* /* context */, TestPtr* ptr)
        {
            result = (*ptr)->getInt();
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
    }
    manager.deleteEntity(2);
    manager.deleteEntity(3);

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

    auto f0index = manager.addForMatchingFunction<EC::Meta::TypeList<C0>>(
            [] (std::size_t /* id */, void* /* context */, C0* c0) {
        ++c0->x;
        ++c0->y;
    });

    auto f1index = manager.addForMatchingFunction<EC::Meta::TypeList<C0, C1>>(
            [] (std::size_t /* id */, void* /* context */, C0* c0, C1* c1) {
        c1->vx = c0->x + 10;
        c1->vy = c1->vy + c1->vx + c0->y + 10;
    });

    auto f2index = manager.addForMatchingFunction<EC::Meta::TypeList<C0>>(
            [] (std::size_t /* id */, void* /* context */, C0* c0) {
        c0->x = c0->y = 9999;
    });

    auto f3index = manager.addForMatchingFunction<EC::Meta::TypeList<C1>>(
            [] (std::size_t /* id */, void* /* context */, C1* c1) {
        c1->vx = c1->vy = 10000;
    });

    EXPECT_EQ(2, manager.removeSomeMatchingFunctions({f2index, f3index}));

    auto f4index = manager.addForMatchingFunction<EC::Meta::TypeList<C0>>(
            [] (std::size_t /* id */, void* /* context */, C0* c0) {
        c0->x = 999;
        c0->y = 888;
    });

    {
        auto set = std::set<std::size_t>({f4index});
        EXPECT_EQ(1, manager.removeSomeMatchingFunctions(set));
    }

    auto f5index = manager.addForMatchingFunction<EC::Meta::TypeList<C0>>(
            [] (std::size_t /* id */, void* /* context */, C0* c0) {
        c0->x = 777;
        c0->y = 666;
    });

    auto lastIndex = f5index;

    {
        auto set = std::unordered_set<std::size_t>({f5index});
        EXPECT_EQ(1, manager.removeSomeMatchingFunctions(set));
    }

    manager.callForMatchingFunctions();

    {
        auto* c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0->x);
        EXPECT_EQ(2, c0->y);

        auto* c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1->vx);
        EXPECT_EQ(23, c1->vy);
    }

    EXPECT_TRUE(manager.callForMatchingFunction(f0index));
    EXPECT_FALSE(manager.callForMatchingFunction(lastIndex + 1));

    {
        auto* c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(2, c0->x);
        EXPECT_EQ(3, c0->y);

        c0->x = 1;
        c0->y = 2;

        auto* c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1->vx);
        EXPECT_EQ(23, c1->vy);
    }

    EXPECT_EQ(1, manager.keepSomeMatchingFunctions({f1index}));

    {
        std::vector<std::size_t> indices{f1index};
        EXPECT_EQ(0, manager.keepSomeMatchingFunctions(indices));
    }
    {
        std::set<std::size_t> indices{f1index};
        EXPECT_EQ(0, manager.keepSomeMatchingFunctions(indices));
    }
    {
        std::unordered_set<std::size_t> indices{f1index};
        EXPECT_EQ(0, manager.keepSomeMatchingFunctions(indices));
    }

    manager.callForMatchingFunctions();

    {
        auto* c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0->x);
        EXPECT_EQ(2, c0->y);

        auto* c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1->vx);
        EXPECT_EQ(46, c1->vy);
    }

    EXPECT_TRUE(manager.removeForMatchingFunction(f1index));
    EXPECT_FALSE(manager.removeForMatchingFunction(f1index));

    manager.callForMatchingFunctions();

    {
        auto* c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0->x);
        EXPECT_EQ(2, c0->y);

        auto* c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1->vx);
        EXPECT_EQ(46, c1->vy);
    }

    manager.clearForMatchingFunctions();
    manager.callForMatchingFunctions();

    {
        auto* c0 = manager.getEntityData<C0>(eid);

        EXPECT_EQ(1, c0->x);
        EXPECT_EQ(2, c0->y);

        auto* c1 = manager.getEntityData<C1>(eid);

        EXPECT_EQ(11, c1->vx);
        EXPECT_EQ(46, c1->vy);
    }
}

TEST(EC, DeletedEntityID)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    auto e0 = manager.addEntity();
    auto e1 = manager.addEntity();
    auto e2 = manager.addEntity();
    auto e3 = manager.addEntity();

    manager.deleteEntity(e0);
    manager.deleteEntity(e1);

    EXPECT_FALSE(manager.isAlive(e0));
    EXPECT_FALSE(manager.isAlive(e1));
    EXPECT_TRUE(manager.isAlive(e2));
    EXPECT_TRUE(manager.isAlive(e3));
}

TEST(EC, MultiThreaded)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    for(unsigned int i = 0; i < 17; ++i)
    {
        manager.addEntity();
        manager.addComponent<C0>(i, 0, 0);
        EXPECT_EQ(0, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(0, manager.getEntityData<C0>(i)->y);
    }

    manager.forMatchingSignature<EC::Meta::TypeList<C0> >(
        [] (const std::size_t& /* id */, void* /* context */, C0* c) {
            c->x = 1;
            c->y = 2;
        },
        nullptr,
        true
    );

    for(unsigned int i = 0; i < 17; ++i)
    {
        EXPECT_EQ(1, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(2, manager.getEntityData<C0>(i)->y);
    }

    for(unsigned int i = 3; i < 17; ++i)
    {
        manager.deleteEntity(i);
    }

    for(unsigned int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(1, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(2, manager.getEntityData<C0>(i)->y);
    }

    manager.forMatchingSignature<EC::Meta::TypeList<C0> >(
        [] (const std::size_t& /* id */, void* /* context */, C0* c) {
            c->x = 3;
            c->y = 4;
        },
        nullptr,
        true
    );

    for(unsigned int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(3, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(4, manager.getEntityData<C0>(i)->y);
    }

    manager.reset();

    for(unsigned int i = 0; i < 17; ++i)
    {
        manager.addEntity();
        manager.addComponent<C0>(i, 0, 0);
        EXPECT_EQ(0, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(0, manager.getEntityData<C0>(i)->y);
    }

    auto f0 = manager.addForMatchingFunction<EC::Meta::TypeList<C0> >(
        [] (const std::size_t& /* id */, void* /* context */, C0* c) {
            c->x = 1;
            c->y = 2;
        }
    );

    manager.callForMatchingFunctions(true);

    for(unsigned int i = 0; i < 17; ++i)
    {
        EXPECT_EQ(1, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(2, manager.getEntityData<C0>(i)->y);
    }

    auto f1 = manager.addForMatchingFunction<EC::Meta::TypeList<C0> >(
        [] (const std::size_t& /* id */, void* /* context */, C0* c) {
            c->x = 3;
            c->y = 4;
        }
    );

    manager.callForMatchingFunction(f1, true);

    for(unsigned int i = 0; i < 17; ++i)
    {
        EXPECT_EQ(3, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(4, manager.getEntityData<C0>(i)->y);
    }

    for(unsigned int i = 4; i < 17; ++i)
    {
        manager.deleteEntity(i);
    }

    manager.callForMatchingFunction(f0, true);

    for(unsigned int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(1, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(2, manager.getEntityData<C0>(i)->y);
    }

    manager.callForMatchingFunction(f1, 8);

    for(unsigned int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(3, manager.getEntityData<C0>(i)->x);
        EXPECT_EQ(4, manager.getEntityData<C0>(i)->y);
    }
}

TEST(EC, ForMatchingSignatures)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    std::size_t e[] = {
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity()
    };

    auto& first = e[0];
    auto& last = e[6];

    for(auto id : e)
    {
        if(id != first && id != last)
        {
            manager.addComponent<C0>(id);
            manager.addComponent<C1>(id);
            manager.addTag<T0>(id);

            auto* c1 = manager.getEntityData<C1>(id);
            c1->vx = 0;
            c1->vy = 0;
        }
        else
        {
            manager.addComponent<C0>(id);
        }
    }

    using namespace EC::Meta;

    manager.forMatchingSignatures<
        TypeList<TypeList<C0>, TypeList<C0, C1> >
    >(
        std::make_tuple(
        [] (std::size_t /* id */, void* /* context */, C0* c) {
            EXPECT_EQ(c->x, 0);
            EXPECT_EQ(c->y, 0);
            c->x = 1;
            c->y = 1;
        },
        [] (std::size_t /* id */, void* /* context */, C0* c0, C1* c1) {
            EXPECT_EQ(c0->x, 1);
            EXPECT_EQ(c0->y, 1);
            EXPECT_EQ(c1->vx, 0);
            EXPECT_EQ(c1->vy, 0);
            c1->vx = c0->x;
            c1->vy = c0->y;
            c0->x = 2;
            c0->y = 2;
        })
    );

    for(auto id : e)
    {
        if(id != first && id != last)
        {
            EXPECT_EQ(2, manager.getEntityData<C0>(id)->x);
            EXPECT_EQ(2, manager.getEntityData<C0>(id)->y);
            EXPECT_EQ(1, manager.getEntityData<C1>(id)->vx);
            EXPECT_EQ(1, manager.getEntityData<C1>(id)->vy);
        }
        else
        {
            EXPECT_EQ(1, manager.getEntityData<C0>(id)->x);
            EXPECT_EQ(1, manager.getEntityData<C0>(id)->y);
        }
    }

    {
    std::unordered_map<std::size_t,int> cx;
    std::unordered_map<std::size_t,int> cy;
    std::unordered_map<std::size_t,int> c0x;
    std::unordered_map<std::size_t,int> c0y;
    std::unordered_map<std::size_t,int> c1vx;
    std::unordered_map<std::size_t,int> c1vy;
    std::mutex cxM;
    std::mutex cyM;
    std::mutex c0xM;
    std::mutex c0yM;
    std::mutex c1vxM;
    std::mutex c1vyM;

    manager.forMatchingSignatures<
        TypeList<TypeList<C0>, TypeList<C0, C1> >
    >
    (
        std::make_tuple(
        [&first, &last, &cx, &cy, &cxM, &cyM]
        (std::size_t eid, void* /* context */, C0* c) {
            if(eid != first && eid != last)
            {
                {
                    std::lock_guard<std::mutex> guard(cxM);
                    cx.insert(std::make_pair(eid, c->x));
                }
                {
                    std::lock_guard<std::mutex> guard(cyM);
                    cy.insert(std::make_pair(eid, c->y));
                }
                c->x = 5;
                c->y = 7;
            }
            else
            {
                {
                    std::lock_guard<std::mutex> guard(cxM);
                    cx.insert(std::make_pair(eid, c->x));
                }
                {
                    std::lock_guard<std::mutex> guard(cyM);
                    cy.insert(std::make_pair(eid, c->y));
                }
                c->x = 11;
                c->y = 13;
            }
        },
        [&c0x, &c0y, &c1vx, &c1vy, &c0xM, &c0yM, &c1vxM, &c1vyM]
        (std::size_t eid, void* /* context */, C0* c0, C1* c1) {
            {
                std::lock_guard<std::mutex> guard(c0xM);
                c0x.insert(std::make_pair(eid, c0->x));
            }
            {
                std::lock_guard<std::mutex> guard(c0yM);
                c0y.insert(std::make_pair(eid, c0->y));
            }
            {
                std::lock_guard<std::mutex> guard(c1vxM);
                c1vx.insert(std::make_pair(eid, c1->vx));
            }
            {
                std::lock_guard<std::mutex> guard(c1vyM);
                c1vy.insert(std::make_pair(eid, c1->vy));
            }

            c1->vx += c0->x;
            c1->vy += c0->y;

            c0->x = 1;
            c0->y = 2;
        }),
        nullptr,
        true
    );
    
    for(auto iter = cx.begin(); iter != cx.end(); ++iter)
    {
        if(iter->first != first && iter->first != last)
        {
            EXPECT_EQ(2, iter->second);
        }
        else
        {
            EXPECT_EQ(1, iter->second);
        }
    }
    for(auto iter = cy.begin(); iter != cy.end(); ++iter)
    {
        if(iter->first != first && iter->first != last)
        {
            EXPECT_EQ(2, iter->second);
        }
        else
        {
            EXPECT_EQ(1, iter->second);
        }
    }
    for(auto iter = c0x.begin(); iter != c0x.end(); ++iter)
    {
        EXPECT_EQ(5, iter->second);
    }
    for(auto iter = c0y.begin(); iter != c0y.end(); ++iter)
    {
        EXPECT_EQ(7, iter->second);
    }
    for(auto iter = c1vx.begin(); iter != c1vx.end(); ++iter)
    {
        EXPECT_EQ(1, iter->second);
    }
    for(auto iter = c1vy.begin(); iter != c1vy.end(); ++iter)
    {
        EXPECT_EQ(1, iter->second);
    }
    }

    for(auto eid : e)
    {
        if(eid != first && eid != last)
        {
            EXPECT_EQ(1, manager.getEntityData<C0>(eid)->x);
            EXPECT_EQ(2, manager.getEntityData<C0>(eid)->y);
            EXPECT_EQ(6, manager.getEntityData<C1>(eid)->vx);
            EXPECT_EQ(8, manager.getEntityData<C1>(eid)->vy);
        }
        else
        {
            EXPECT_EQ(11, manager.getEntityData<C0>(eid)->x);
            EXPECT_EQ(13, manager.getEntityData<C0>(eid)->y);
        }
    }

    // test duplicate signatures
    manager.forMatchingSignatures<TypeList<
        TypeList<C0, C1>,
        TypeList<C0, C1> > >(
        std::make_tuple(
            [] (std::size_t /* id */, void* /* context */, C0* c0, C1* c1) {
                c0->x = 9999;
                c0->y = 9999;
                c1->vx = 9999;
                c1->vy = 9999;
            },
            [] (std::size_t /* id */, void* /* context */, C0* c0, C1* c1) {
                c0->x = 10000;
                c0->y = 10000;
                c1->vx = 10000;
                c1->vy = 10000;
            }
    ));
    for(auto id : e)
    {
        if(id != first && id != last)
        {
            EXPECT_EQ(10000, manager.getEntityData<C0>(id)->x);
            EXPECT_EQ(10000, manager.getEntityData<C0>(id)->y);
            EXPECT_EQ(10000, manager.getEntityData<C1>(id)->vx);
            EXPECT_EQ(10000, manager.getEntityData<C1>(id)->vy);
        }
    };
}

TEST(EC, forMatchingPtrs)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    std::size_t e[] = {
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity(),
        manager.addEntity()
    };

    auto& first = e[0];
    auto& last = e[6];

    for(auto eid : e)
    {
        if(eid != first && eid != last)
        {
            manager.addComponent<C0>(eid);
            manager.addComponent<C1>(eid);
        }
        else
        {
            manager.addComponent<C0>(eid);
            manager.addTag<T0>(eid);
        }
    }

    const auto func0 = []
        (std::size_t /* id */, void* /* context */, C0* c0, C1* c1)
    {
        c0->x = 1;
        c0->y = 2;
        c1->vx = 3;
        c1->vy = 4;
    };
    const auto func1 = [] (std::size_t /* id */, void* /* context */, C0* c0)
    {
        c0->x = 11;
        c0->y = 12;
    };

    using namespace EC::Meta;

    manager.forMatchingSignaturePtr<TypeList<C0, C1> >(
        &func0
    );
    manager.forMatchingSignaturePtr<TypeList<C0, T0> >(
        &func1, nullptr, true
    );

    for(auto eid : e)
    {
        if(eid != first && eid != last)
        {
            C0* c0 = manager.getEntityData<C0>(eid);
            EXPECT_EQ(1, c0->x);
            EXPECT_EQ(2, c0->y);
            c0->x = 0;
            c0->y = 0;
            C1* c1 = manager.getEntityData<C1>(eid);
            EXPECT_EQ(3, c1->vx);
            EXPECT_EQ(4, c1->vy);
            c1->vx = 0;
            c1->vy = 0;
        }
        else
        {
            C0* c = manager.getEntityData<C0>(eid);
            EXPECT_EQ(11, c->x);
            EXPECT_EQ(12, c->y);
            c->x = 0;
            c->y = 0;
        }
    }

    manager.forMatchingSignaturesPtr<TypeList<
        TypeList<C0, C1>,
        TypeList<C0, T0> > >(
        std::make_tuple(&func0, &func1)
    );

    for(auto eid : e)
    {
        if(eid != first && eid != last)
        {
            C0* c0 = manager.getEntityData<C0>(eid);
            EXPECT_EQ(1, c0->x);
            EXPECT_EQ(2, c0->y);
            c0->x = 0;
            c0->y = 0;
            C1* c1 = manager.getEntityData<C1>(eid);
            EXPECT_EQ(3, c1->vx);
            EXPECT_EQ(4, c1->vy);
            c1->vx = 0;
            c1->vy = 0;
        }
        else
        {
            C0* c = manager.getEntityData<C0>(eid);
            EXPECT_EQ(11, c->x);
            EXPECT_EQ(12, c->y);
            c->x = 0;
            c->y = 0;
        }
    }

    // test duplicate signatures
    const auto setTo9999 = []
        (std::size_t /* id */, void* /* context */, C0* c0, C1* c1)
    {
        c0->x = 9999;
        c0->y = 9999;
        c1->vx = 9999;
        c1->vy = 9999;
    };
    const auto setTo10000 = []
        (std::size_t /* id */, void* /* context */, C0* c0, C1* c1)
    {
        c0->x = 10000;
        c0->y = 10000;
        c1->vx = 10000;
        c1->vy = 10000;
    };
    manager.forMatchingSignaturesPtr<TypeList<
        TypeList<C0, C1>,
        TypeList<C0, C1> > >(
        std::make_tuple(
            &setTo9999,
            &setTo10000
    ));
    for(auto id : e)
    {
        if(id != first && id != last)
        {
            EXPECT_EQ(10000, manager.getEntityData<C0>(id)->x);
            EXPECT_EQ(10000, manager.getEntityData<C0>(id)->y);
            EXPECT_EQ(10000, manager.getEntityData<C1>(id)->vx);
            EXPECT_EQ(10000, manager.getEntityData<C1>(id)->vy);
        }
    };
}

TEST(EC, context)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;
    auto e0 = manager.addEntity();
    auto e1 = manager.addEntity();

    manager.addComponent<C0>(e0, 1, 2);
    manager.addComponent<C0>(e1, 3, 4);

    Context c;
    c.a = 2000;
    c.b = 5432;

    EXPECT_EQ(1, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(2, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(3, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(4, manager.getEntityData<C0>(e1)->y);

    manager.forMatchingSignature<EC::Meta::TypeList<C0>>(assignContextToC0, &c);

    EXPECT_EQ(2000, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(5432, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(2000, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(5432, manager.getEntityData<C0>(e1)->y);

    c.a = 1111;
    c.b = 2222;

    using C0TL = EC::Meta::TypeList<C0>;
    manager.forMatchingSignatures<EC::Meta::TypeList<C0TL, C0TL, C0TL>>(
        std::make_tuple(
            setC0ToOneAndTwo, assignContextToC0, setContextToThreeAndFour),
        &c);
    EXPECT_EQ(1111, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(2222, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(1111, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(2222, manager.getEntityData<C0>(e1)->y);

    EXPECT_EQ(3, c.a);
    EXPECT_EQ(4, c.b);

    manager.forMatchingSignaturesPtr<EC::Meta::TypeList<C0TL, C0TL>>(
        std::make_tuple(
            &setC0ToOneAndTwo, &assignC0ToContext),
        &c);

    EXPECT_EQ(1, c.a);
    EXPECT_EQ(2, c.b);

    EXPECT_EQ(1, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(2, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(1, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(2, manager.getEntityData<C0>(e1)->y);

    c.a = 1980;
    c.b = 1990;

    auto fid = manager.addForMatchingFunction<C0TL>(assignContextToC0, &c);

    manager.callForMatchingFunction(fid);
    EXPECT_EQ(1980, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(1990, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(1980, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(1990, manager.getEntityData<C0>(e1)->y);

    c.a = 2000;
    c.b = 2010;

    manager.callForMatchingFunctions();
    EXPECT_EQ(2000, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(2010, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(2000, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(2010, manager.getEntityData<C0>(e1)->y);

    Context altC;
    altC.a = 999;
    altC.b = 1999;

    manager.changeForMatchingFunctionContext(fid, &altC);
    manager.callForMatchingFunctions();
    EXPECT_EQ(999, manager.getEntityData<C0>(e0)->x);
    EXPECT_EQ(1999, manager.getEntityData<C0>(e0)->y);
    EXPECT_EQ(999, manager.getEntityData<C0>(e1)->x);
    EXPECT_EQ(1999, manager.getEntityData<C0>(e1)->y);
}

TEST(EC, FunctionStorageOrder)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;
    auto e0 = manager.addEntity();
    manager.addComponent<C0>(e0, 1, 2);

    std::vector<int> v;

    using C0TL = EC::Meta::TypeList<C0>;

    manager.addForMatchingFunction<C0TL>(
        [&v] (std::size_t /* id */, void* /* context */, C0* c) {
            v.push_back(c->x);
            v.push_back(c->y);
        });
    manager.addForMatchingFunction<C0TL>(
        [] (std::size_t /* id */, void* /* context */, C0* c) {
            c->x += 2;
            c->y += 2;
        });
    manager.addForMatchingFunction<C0TL>(
        [&v] (std::size_t /* id */, void* /* context */, C0* c) {
            v.push_back(c->x);
            v.push_back(c->y);
        });
    manager.addForMatchingFunction<C0TL>(
        [] (std::size_t /* id */, void* /* context */, C0* c) {
            c->x += 2;
            c->y += 2;
        });
    manager.addForMatchingFunction<C0TL>(
        [&v] (std::size_t /* id */, void* /* context */, C0* c) {
            v.push_back(c->x);
            v.push_back(c->y);
        });

    manager.callForMatchingFunctions();

    EXPECT_EQ(1, v.at(0));
    EXPECT_EQ(2, v.at(1));
    EXPECT_EQ(3, v.at(2));
    EXPECT_EQ(4, v.at(3));
    EXPECT_EQ(5, v.at(4));
    EXPECT_EQ(6, v.at(5));
}

TEST(EC, forMatchingSimple) {
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    auto e0 = manager.addEntity();
    manager.addComponent<C0>(e0, 0, 1);

    auto e1 = manager.addEntity();
    manager.addComponent<C0>(e1, 2, 3);
    manager.addTag<T0>(e1);

    auto e2 = manager.addEntity();
    manager.addComponent<C0>(e2, 4, 5);
    manager.addTag<T0>(e2);
    manager.addTag<T1>(e2);

    // add 10 to C0 components
    manager.forMatchingSimple<EC::Meta::TypeList<C0>>(
        [] (std::size_t id, decltype(manager) *manager, void *) {
            C0 *c0 = manager->getEntityData<C0>(id);
            c0->x += 10;
            c0->y += 10;
        }, nullptr, true);

    // verify
    {
        C0 *c0 = manager.getEntityData<C0>(e0);
        EXPECT_EQ(c0->x, 10);
        EXPECT_EQ(c0->y, 11);
        c0 = manager.getEntityData<C0>(e1);
        EXPECT_EQ(c0->x, 12);
        EXPECT_EQ(c0->y, 13);
        c0 = manager.getEntityData<C0>(e2);
        EXPECT_EQ(c0->x, 14);
        EXPECT_EQ(c0->y, 15);
    }

    auto e3 = manager.addEntity();
    manager.addComponent<C0>(e3, 6, 7);
    manager.addTag<T0>(e3);
    manager.addTag<T1>(e3);

    // add 100 to entities with C0,T1
    manager.forMatchingSimple<EC::Meta::TypeList<C0, T1>>(
        [] (std::size_t id, decltype(manager) *manager, void *) {
            C0 *c0 = manager->getEntityData<C0>(id);
            c0->x += 100;
            c0->y += 100;
        });

    // verify
    {
        C0 *c0 = manager.getEntityData<C0>(e0);
        EXPECT_EQ(c0->x, 10);
        EXPECT_EQ(c0->y, 11);
        c0 = manager.getEntityData<C0>(e1);
        EXPECT_EQ(c0->x, 12);
        EXPECT_EQ(c0->y, 13);
        c0 = manager.getEntityData<C0>(e2);
        EXPECT_EQ(c0->x, 114);
        EXPECT_EQ(c0->y, 115);
        c0 = manager.getEntityData<C0>(e3);
        EXPECT_EQ(c0->x, 106);
        EXPECT_EQ(c0->y, 107);
    }
}

TEST(EC, forMatchingIterableFn)
{
    EC::Manager<ListComponentsAll, ListTagsAll> manager;
    auto e0 = manager.addEntity();
    manager.addComponent<C0>(e0, 0, 1);

    auto e1 = manager.addEntity();
    manager.addComponent<C0>(e1, 2, 3);
    manager.addTag<T0>(e1);

    auto e2 = manager.addEntity();
    manager.addComponent<C0>(e2, 4, 5);
    manager.addTag<T0>(e2);
    manager.addTag<T1>(e2);

    auto c0Index = EC::Meta::IndexOf<C0, ListCombinedComponentsTags>::value;
    auto c1Index = EC::Meta::IndexOf<C1, ListCombinedComponentsTags>::value;
    auto t0Index = EC::Meta::IndexOf<T0, ListCombinedComponentsTags>::value;
    auto t1Index = EC::Meta::IndexOf<T1, ListCombinedComponentsTags>::value;

    {
        // test valid indices
        auto iterable = {c0Index};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto* c = m->getEntityComponent<C0>(i);
            c->x += 1;
            c->y += 1;
        };
        manager.forMatchingIterable(iterable, fn, nullptr);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 1);
        EXPECT_EQ(c->y, 2);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 3);
        EXPECT_EQ(c->y, 4);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 5);
        EXPECT_EQ(c->y, 6);
    }

    {
        // test invalid indices
        auto iterable = {c0Index, c1Index};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto* c = m->getEntityComponent<C0>(i);
            c->x += 1;
            c->y += 1;
        };
        manager.forMatchingIterable(iterable, fn, nullptr);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 1);
        EXPECT_EQ(c->y, 2);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 3);
        EXPECT_EQ(c->y, 4);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 5);
        EXPECT_EQ(c->y, 6);
    }

    {
        // test partially valid indices
        auto iterable = {c0Index, t1Index};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto* c = m->getEntityComponent<C0>(i);
            c->x += 1;
            c->y += 1;
        };
        manager.forMatchingIterable(iterable, fn, nullptr);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 1);
        EXPECT_EQ(c->y, 2);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 3);
        EXPECT_EQ(c->y, 4);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 6);
        EXPECT_EQ(c->y, 7);
    }

    {
        // test partially valid indices
        auto iterable = {c0Index, t0Index};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto* c = m->getEntityComponent<C0>(i);
            c->x += 10;
            c->y += 10;
        };
        manager.forMatchingIterable(iterable, fn, nullptr);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 1);
        EXPECT_EQ(c->y, 2);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 13);
        EXPECT_EQ(c->y, 14);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 16);
        EXPECT_EQ(c->y, 17);
    }

    {
        // test invalid indices
        auto iterable = {(unsigned int)c0Index, 1000u};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto* c = m->getEntityComponent<C0>(i);
            c->x += 1000;
            c->y += 1000;
        };
        manager.forMatchingIterable(iterable, fn, nullptr);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 1);
        EXPECT_EQ(c->y, 2);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 13);
        EXPECT_EQ(c->y, 14);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 16);
        EXPECT_EQ(c->y, 17);
    }

    {
        // test concurrent update
        auto iterable = {c0Index};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto *c = m->getEntityComponent<C0>(i);
            c->x += 100;
            c->y += 100;
        };
        manager.forMatchingIterable(iterable, fn, nullptr, true);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 101);
        EXPECT_EQ(c->y, 102);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 113);
        EXPECT_EQ(c->y, 114);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 116);
        EXPECT_EQ(c->y, 117);
    }


    {
        // test invalid concurrent update
        auto iterable = {(unsigned int)c0Index, 1000u};
        auto fn = [] (std::size_t i, decltype(manager)* m, void*) {
            auto *c = m->getEntityComponent<C0>(i);
            c->x += 1000;
            c->y += 1000;
        };
        manager.forMatchingIterable(iterable, fn, nullptr, true);
    }

    {
        auto* c = manager.getEntityComponent<C0>(e0);
        EXPECT_EQ(c->x, 101);
        EXPECT_EQ(c->y, 102);

        c = manager.getEntityComponent<C0>(e1);
        EXPECT_EQ(c->x, 113);
        EXPECT_EQ(c->y, 114);

        c = manager.getEntityComponent<C0>(e2);
        EXPECT_EQ(c->x, 116);
        EXPECT_EQ(c->y, 117);
    }
}

// This test tests for the previously fixed bug where
// callForMatchingFunction(s) can fail to call fns on
// the correct entities.
// Fixed in commit e0f30db951fcedd0ec51c680bd60a2157bd355a6
TEST(EC, MultiThreadedForMatching) {
    EC::Manager<ListComponentsAll, ListTagsAll> manager;

    std::size_t first = manager.addEntity();
    std::size_t second = manager.addEntity();

    manager.addComponent<C1>(second);
    manager.getEntityComponent<C1>(second)->vx = 1;

    std::size_t fnIdx = manager.addForMatchingFunction<EC::Meta::TypeList<C1>>(
            [] (std::size_t id, void *data, C1 *c) {
        c->vx -= 1;
        if(c->vx <= 0) {
            auto *manager = (EC::Manager<ListComponentsAll, ListTagsAll>*)data;
            manager->deleteEntity(id);
        }
    }, &manager);

    EXPECT_TRUE(manager.isAlive(first));
    EXPECT_TRUE(manager.isAlive(second));

    manager.callForMatchingFunction(fnIdx, true);

    EXPECT_TRUE(manager.isAlive(first));
    EXPECT_FALSE(manager.isAlive(second));
}

TEST(EC, ManagerWithLowThreadCount) {
    EC::Manager<ListComponentsAll, ListTagsAll, 1> manager;

    std::array<std::size_t, 10> entities;
    for(auto &id : entities) {
        id = manager.addEntity();
        manager.addComponent<C0>(id);
    }

    for(const auto &id : entities) {
        auto *component = manager.getEntityComponent<C0>(id);
        EXPECT_EQ(component->x, 0);
        EXPECT_EQ(component->y, 0);
    }

    manager.forMatchingSignature<EC::Meta::TypeList<C0> >([] (std::size_t /*id*/, void* /*ud*/, C0 *c) {
        c->x += 1;
        c->y += 1;
    }, nullptr, true);

    for(const auto &id : entities) {
        auto *component = manager.getEntityComponent<C0>(id);
        EXPECT_EQ(component->x, 1);
        EXPECT_EQ(component->y, 1);
    }
}

TEST(EC, ManagerDeferredDeletions) {
    using ManagerType = EC::Manager<ListComponentsAll, ListTagsAll, 8>;
    ManagerType manager;

    std::array<std::size_t, 24> entities;
    for(std::size_t i = 0; i < entities.size(); ++i) {
        entities.at(i) = manager.addEntity();
        manager.addTag<T0>(entities.at(i));
        if(i < entities.size() / 2) {
            manager.addTag<T1>(entities.at(i));
        }
    }

    auto dataTuple = std::tuple<decltype(manager)*, std::size_t>
        {&manager, entities.size()};

    manager.forMatchingSignature<EC::Meta::TypeList<T0, T1>>([] (std::size_t id, void *data) {
        auto *tuple = (std::tuple<ManagerType*, std::size_t>*)data;
        std::size_t size = std::get<1>(*tuple);
        std::get<0>(*tuple)->deleteEntity(id + size / 4);
    }, &dataTuple, true);

    for(std::size_t i = 0; i < entities.size(); ++i) {
        if (entities.at(i) >= entities.size() / 4
                && entities.at(i) < entities.size() * 3 / 4) {
            EXPECT_FALSE(manager.isAlive(entities.at(i)));
        } else {
            EXPECT_TRUE(manager.isAlive(entities.at(i)));
        }
    }
}

TEST(EC, NestedThreadPoolTasks) {
    using ManagerType = EC::Manager<ListComponentsAll, ListTagsAll, 2>;
    ManagerType manager;

    std::array<std::size_t, 64> entities;
    for (auto &entity : entities) {
        entity = manager.addEntity();
        manager.addComponent<C0>(entity, entity, entity);
    }

    manager.forMatchingSignature<EC::Meta::TypeList<C0>>([] (std::size_t id, void *data, C0 *c) {
        ManagerType *manager = (ManagerType*)data;

        manager->forMatchingSignature<EC::Meta::TypeList<C0>>([id] (std::size_t inner_id, void* data, C0 *inner_c) {
            const C0 *const outer_c = (C0*)data;
            EXPECT_EQ(id, outer_c->x);
            EXPECT_EQ(inner_id, inner_c->x);
            if (id == inner_id) {
                EXPECT_EQ(outer_c->x, inner_c->x);
                EXPECT_EQ(outer_c->y, inner_c->y);
            } else {
                EXPECT_NE(outer_c->x, inner_c->x);
                EXPECT_NE(outer_c->y, inner_c->y);
            }
        }, c, false);
    }, &manager, true);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}
