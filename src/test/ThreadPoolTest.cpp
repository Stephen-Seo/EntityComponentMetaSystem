#include "test_helpers.h"

#include <EC/ThreadPool.hpp>

using OneThreadPool = EC::ThreadPool<1>;
using ThreeThreadPool = EC::ThreadPool<3>;

void TEST_ECThreadPool_OneThread() {
    OneThreadPool p;
    std::atomic_int data;
    data.store(0);
    const auto fn = [](void *ud) {
        auto *data = static_cast<std::atomic_int*>(ud);
        data->fetch_add(1);
    };

    p.queueFn(fn, &data);

    p.startThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isQueueEmpty() || !p.isNotRunning());

    ASSERT_EQ(data.load(), 1);

    for(unsigned int i = 0; i < 10; ++i) {
        p.queueFn(fn, &data);
    }
    p.startThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isQueueEmpty() || !p.isNotRunning());

    ASSERT_EQ(data.load(), 11);
}

void TEST_ECThreadPool_Simple() {
    ThreeThreadPool p;
    std::atomic_int data;
    data.store(0);
    const auto fn = [](void *ud) {
        auto *data = static_cast<std::atomic_int*>(ud);
        data->fetch_add(1);
    };

    p.queueFn(fn, &data);

    p.startThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isQueueEmpty() || !p.isNotRunning());

    ASSERT_EQ(data.load(), 1);

    for(unsigned int i = 0; i < 10; ++i) {
        p.queueFn(fn, &data);
    }
    p.startThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isQueueEmpty() || !p.isNotRunning());

    ASSERT_EQ(data.load(), 11);
}

void TEST_ECThreadPool_QueryCount() {
    {
        OneThreadPool oneP;
        ASSERT_EQ(1, oneP.getMaxThreadCount());
    }
    {
        ThreeThreadPool threeP;
        ASSERT_EQ(3, threeP.getMaxThreadCount());
    }
}

void TEST_ECThreadPool_easyStartAndWait() {
    std::atomic_int data;
    data.store(0);
    {
        OneThreadPool oneP;
        for(unsigned int i = 0; i < 20; ++i) {
            oneP.queueFn([] (void *ud) {
                auto *atomicInt = static_cast<std::atomic_int*>(ud);
                atomicInt->fetch_add(1);
            }, &data);
        }
        oneP.easyStartAndWait();
        CHECK_EQ(20, data.load());
    }
    {
        ThreeThreadPool threeP;
        for(unsigned int i = 0; i < 20; ++i) {
            threeP.queueFn([] (void *ud) {
                auto *atomicInt = static_cast<std::atomic_int*>(ud);
                atomicInt->fetch_add(1);
            }, &data);
        }
        threeP.easyStartAndWait();
        CHECK_EQ(40, data.load());
    }
}
