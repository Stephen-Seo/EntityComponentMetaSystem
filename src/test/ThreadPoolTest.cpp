#include <gtest/gtest.h>

#include <EC/ThreadPool.hpp>

//using OneThreadPool = EC::ThreadPool<1>;
using ThreeThreadPool = EC::ThreadPool<3>;

//TEST(ECThreadPool, CannotCompile) {
//    OneThreadPool tp;
//}

TEST(ECThreadPool, Simple) {
    ThreeThreadPool p{};
    std::atomic_int data;
    data.store(0);
    const auto fn = [](void *ud) {
        auto *data = static_cast<std::atomic_int*>(ud);
        data->fetch_add(1);
    };

    p.queueFn(fn, &data);

    p.wakeThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isAllThreadsWaiting());

    ASSERT_EQ(data.load(), 1);

    for(unsigned int i = 0; i < 10; ++i) {
        p.queueFn(fn, &data);
    }
    p.wakeThreads();

    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while(!p.isAllThreadsWaiting());

    ASSERT_EQ(data.load(), 11);
}
