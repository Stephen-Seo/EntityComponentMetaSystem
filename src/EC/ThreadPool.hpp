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

/*!
    \brief Implementation of a Thread Pool.

    Note that if SIZE is less than 2, then ThreadPool will not create threads and
    run queued functions on the calling thread.
*/
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

    /*!
        \brief Queues a function to be called (doesn't start calling yet).

        To run the queued functions, wakeThreads() must be called to wake the
        waiting threads which will start pulling functions from the queue to be
        called.
    */
    void queueFn(std::function<void(void*)>&& fn, void *ud = nullptr) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fnQueue.emplace(std::make_tuple(fn, ud));
    }

    /*!
        \brief Wakes waiting threads to start running queued functions.

        If SIZE is less than 2, then this function call will block until all the
        queued functions have been executed on the calling thread.

        If SIZE is 2 or greater, then this function will return immediately after
        waking one or all threads, depending on the given boolean parameter.
    */
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

    /*!
        \brief Gets the number of waiting threads.

        If all threads are waiting, this should equal ThreadCount.

        If SIZE is less than 2, then this will always return 0.
    */
    int getWaitCount() {
        std::lock_guard<std::mutex> lock(waitCountMutex);
        return waitCount;
    }

    /*!
        \brief Returns true if all threads are waiting.

        If SIZE is less than 2, then this will always return true.
    */
    bool isAllThreadsWaiting() {
        if(SIZE >= 2) {
            std::lock_guard<std::mutex> lock(waitCountMutex);
            return waitCount == THREADCOUNT::value;
        } else {
            return true;
        }
    }

    /*!
        \brief Returns true if the function queue is empty.
    */
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
