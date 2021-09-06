#ifndef EC_META_SYSTEM_THREADPOOL_HPP
#define EC_META_SYSTEM_THREADPOOL_HPP

#include <type_traits>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <tuple>
#include <chrono>

namespace EC {

namespace Internal {
    using TPFnType = std::function<void(void*)>;
    using TPTupleType = std::tuple<TPFnType, void*>;
    using TPQueueType = std::queue<TPTupleType>;
} // namespace Internal

template <unsigned int SIZE, typename = void>
class ThreadPool;

template <unsigned int SIZE>
class ThreadPool<SIZE, typename std::enable_if<(SIZE >= 2)>::type> {
public:
    using THREADCOUNT = std::integral_constant<int, SIZE>;

    ThreadPool() : waitCount(0) {
        isAlive.store(true);
        for(unsigned int i = 0; i < SIZE; ++i) {
            threads.emplace_back([] (std::atomic_bool *isAlive,
                                     std::condition_variable *cv,
                                     std::mutex *cvMutex,
                                     Internal::TPQueueType *fnQueue,
                                     std::mutex *queueMutex,
                                     int *waitCount,
                                     std::mutex *waitCountMutex) {
                bool hasFn = false;
                Internal::TPTupleType fnTuple;
                while(isAlive->load()) {
                    hasFn = false;
                    {
                        std::lock_guard<std::mutex> lock(*queueMutex);
                        if(!fnQueue->empty()) {
                            fnTuple = fnQueue->front();
                            fnQueue->pop();
                            hasFn = true;
                        }
                    }
                    if(hasFn) {
                        std::get<0>(fnTuple)(std::get<1>(fnTuple));
                        continue;
                    }

                    {
                        std::lock_guard<std::mutex> lock(*waitCountMutex);
                        *waitCount += 1;
                    }
                    {
                        std::unique_lock<std::mutex> lock(*cvMutex);
                        cv->wait(lock);
                    }
                    {
                        std::lock_guard<std::mutex> lock(*waitCountMutex);
                        *waitCount -= 1;
                    }
                }
            }, &isAlive, &cv, &cvMutex, &fnQueue, &queueMutex, &waitCount, &waitCountMutex);
        }
    }

    ~ThreadPool() {
        isAlive.store(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        cv.notify_all();
        for(auto &thread : threads) {
            thread.join();
        }
    }

    void queueFn(std::function<void(void*)>&& fn, void *ud = nullptr) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fnQueue.emplace(std::make_tuple(fn, ud));
    }

    void wakeThreads(bool wakeAll = true) {
        if(wakeAll) {
            cv.notify_all();
        } else {
            cv.notify_one();
        }
    }

    int getWaitCount() {
        std::lock_guard<std::mutex> lock(waitCountMutex);
        return waitCount;
    }

    bool isAllThreadsWaiting() {
        std::lock_guard<std::mutex> lock(waitCountMutex);
        return waitCount == THREADCOUNT::value;
    }

private:
    std::vector<std::thread> threads;
    std::atomic_bool isAlive;
    std::condition_variable cv;
    std::mutex cvMutex;
    Internal::TPQueueType fnQueue;
    std::mutex queueMutex;
    int waitCount;
    std::mutex waitCountMutex;

};

} // namespace EC

#endif
