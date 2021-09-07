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

template <unsigned int SIZE>
class ThreadPool {
public:
    using THREADCOUNT = std::integral_constant<int, SIZE>;

    ThreadPool() : waitCount(0) {
        isAlive.store(true);
        if(SIZE >= 2) {
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
                }, &isAlive, &cv, &cvMutex, &fnQueue, &queueMutex, &waitCount,
                   &waitCountMutex);
            }
        }
    }

    ~ThreadPool() {
        if(SIZE >= 2) {
            isAlive.store(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            cv.notify_all();
            for(auto &thread : threads) {
                thread.join();
            }
        }
    }

    void queueFn(std::function<void(void*)>&& fn, void *ud = nullptr) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fnQueue.emplace(std::make_tuple(fn, ud));
    }

    void wakeThreads(bool wakeAll = true) {
        if(SIZE >= 2) {
            // wake threads to pull functions from queue and run them
            if(wakeAll) {
                cv.notify_all();
            } else {
                cv.notify_one();
            }
        } else {
            // pull functions from queue and run them on main thread
            Internal::TPTupleType fnTuple;
            bool hasFn;
            do {
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if(!fnQueue.empty()) {
                        hasFn = true;
                        fnTuple = fnQueue.front();
                        fnQueue.pop();
                    } else {
                        hasFn = false;
                    }
                }
                if(hasFn) {
                    std::get<0>(fnTuple)(std::get<1>(fnTuple));
                }
            } while(hasFn);
        }
    }

    int getWaitCount() {
        std::lock_guard<std::mutex> lock(waitCountMutex);
        return waitCount;
    }

    bool isAllThreadsWaiting() {
        if(SIZE >= 2) {
            std::lock_guard<std::mutex> lock(waitCountMutex);
            return waitCount == THREADCOUNT::value;
        } else {
            return true;
        }
    }

    bool isQueueEmpty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return fnQueue.empty();
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
