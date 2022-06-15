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
#include <unordered_set>

#ifndef NDEBUG
# include <iostream>
#endif

namespace EC {

namespace Internal {
    using TPFnType = std::function<void(void*)>;
    using TPTupleType = std::tuple<TPFnType, void*>;
    using TPQueueType = std::queue<TPTupleType>;

    template <unsigned int SIZE>
    void thread_fn(std::atomic_bool *isAlive,
                   std::condition_variable *cv,
                   std::mutex *cvMutex,
                   Internal::TPQueueType *fnQueue,
                   std::mutex *queueMutex,
                   std::atomic_int *waitCount) {
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

            waitCount->fetch_add(1);
            {
                std::unique_lock<std::mutex> lock(*cvMutex);
                cv->wait(lock);
            }
            waitCount->fetch_sub(1);
        }
    }
} // namespace Internal

/*!
    \brief Implementation of a Thread Pool.

    Note that if SIZE is less than 2, then ThreadPool will not create threads and
    run queued functions on the calling thread.
*/
template <unsigned int SIZE>
class ThreadPool {
public:
    ThreadPool() {
        waitCount.store(0);
        extraThreadCount.store(0);
        isAlive.store(true);
        if(SIZE >= 2) {
            for(unsigned int i = 0; i < SIZE; ++i) {
                threads.emplace_back(Internal::thread_fn<SIZE>,
                                     &isAlive,
                                     &cv,
                                     &cvMutex,
                                     &fnQueue,
                                     &queueMutex,
                                     &waitCount);
                threadsIDs.insert(threads.back().get_id());
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
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
    void wakeThreads(const bool wakeAll = true) {
        if(SIZE >= 2) {
            // wake threads to pull functions from queue and run them
            if(wakeAll) {
                cv.notify_all();
            } else {
                cv.notify_one();
            }

            // check if all threads are running a task, and spawn a new thread
            // if this is the case
            Internal::TPTupleType fnTuple;
            bool hasFn = false;
            if (waitCount.load(std::memory_order_relaxed) == 0) {
                std::lock_guard<std::mutex> queueLock(queueMutex);
                if (!fnQueue.empty()) {
                    fnTuple = fnQueue.front();
                    fnQueue.pop();
                    hasFn = true;
                }
            }

            if (hasFn) {
#ifndef NDEBUG
                std::cout << "Spawning extra thread...\n";
#endif
                extraThreadCount.fetch_add(1);
                std::thread newThread = std::thread(
                        [] (Internal::TPTupleType &&tuple, std::atomic_int *count) {
                            std::get<0>(tuple)(std::get<1>(tuple));
#ifndef NDEBUG
                            std::cout << "Stopping extra thread...\n";
#endif
                            count->fetch_sub(1);
                        },
                        std::move(fnTuple), &extraThreadCount);
                newThread.detach();
            }
        } else {
            sequentiallyRunTasks();
        }
    }

    /*!
        \brief Gets the number of waiting threads.

        If all threads are waiting, this should equal ThreadCount.

        If SIZE is less than 2, then this will always return 0.
    */
    int getWaitCount() {
        return waitCount.load(std::memory_order_relaxed);
    }

    /*!
        \brief Returns true if all threads are waiting.

        If SIZE is less than 2, then this will always return true.
    */
    bool isAllThreadsWaiting() {
        if(SIZE >= 2) {
            return waitCount.load(std::memory_order_relaxed) == SIZE;
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

    /*!
        \brief Returns the ThreadCount that this class was created with.
     */
    constexpr unsigned int getThreadCount() {
        return SIZE;
    }

    /*!
        \brief Wakes all threads and blocks until all queued tasks are finished.

        If SIZE is less than 2, then this function call will block until all the
        queued functions have been executed on the calling thread.

        If SIZE is 2 or greater, then this function will block until all the
        queued functions have been executed by the threads in the thread pool.
     */
    void easyWakeAndWait() {
        if(SIZE >= 2) {
            do {
                wakeThreads();
                std::this_thread::sleep_for(std::chrono::microseconds(150));
            } while(!isQueueEmpty()
                    || (threadsIDs.find(std::this_thread::get_id()) != threadsIDs.end()
                         && extraThreadCount.load(std::memory_order_relaxed) != 0));
//            } while(!isQueueEmpty() || !isAllThreadsWaiting());
        } else {
            sequentiallyRunTasks();
        }
    }

private:
    std::vector<std::thread> threads;
    std::unordered_set<std::thread::id> threadsIDs;
    std::atomic_bool isAlive;
    std::condition_variable cv;
    std::mutex cvMutex;
    Internal::TPQueueType fnQueue;
    std::mutex queueMutex;
    std::atomic_int waitCount;
    std::atomic_int extraThreadCount;

    void sequentiallyRunTasks() {
        // pull functions from queue and run them on current thread
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

};

} // namespace EC

#endif
