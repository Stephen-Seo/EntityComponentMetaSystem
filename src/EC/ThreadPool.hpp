#ifndef EC_META_SYSTEM_THREADPOOL_HPP
#define EC_META_SYSTEM_THREADPOOL_HPP

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#ifndef NDEBUG
#include <iostream>
#endif

namespace EC {

namespace Internal {
using TPFnType = std::function<void(void *)>;
using TPTupleType = std::tuple<TPFnType, void *>;
using TPQueueType = std::queue<TPTupleType>;
using ThreadOpt = std::optional<std::thread>;
using ThreadStackType = std::vector<std::tuple<ThreadOpt, std::thread::id>>;
using ThreadStacksType = std::deque<ThreadStackType>;
using ThreadStacksMutexesT = std::deque<std::mutex>;
using ThreadCountersT = std::deque<std::atomic_uint>;
using PtrsHoldT = std::deque<std::atomic_bool>;
using PointersT = std::tuple<ThreadStackType *, std::mutex *,
                             std::atomic_uint *, std::atomic_bool *>;
}  // namespace Internal

/*!
    \brief Implementation of a Thread Pool.

    Note that MAXSIZE template parameter determines how many threads are created
    each time that startThreads() (or easyStartAndWait()) is called.
*/
template <unsigned int MAXSIZE>
class ThreadPool {
   public:
    ThreadPool()
        : threadStacks{}, threadStackMutexes{}, fnQueue{}, queueMutex{} {}

    ~ThreadPool() {
        while (!isNotRunning()) {
            std::this_thread::sleep_for(std::chrono::microseconds(30));
        }
    }

    /*!
        \brief Queues a function to be called (doesn't start calling yet).

        To run the queued functions, startThreads() must be called to wake the
        waiting threads which will start pulling functions from the queue to be
        called.

        Note that the easyStartAndWait() calls startThreads() and waits until
        the threads have finished execution.
    */
    void queueFn(std::function<void(void *)> &&fn, void *ud = nullptr) {
        std::lock_guard<std::mutex> lock(queueMutex);
        fnQueue.emplace(std::make_tuple(fn, ud));
    }

    /*!
        \brief Creates MAXSIZE threads that will process queueFn() functions.

        Note that if MAXSIZE < 2, then this function will synchronously execute
        the queued functions and block until the functions have been executed.
        Otherwise, this function may return before the queued functions have
        been executed.
     */
    Internal::PointersT startThreads() {
        if constexpr (MAXSIZE >= 2) {
            checkStacks();
            auto pointers = newStackEntry();
            Internal::ThreadStackType *threadStack = std::get<0>(pointers);
            std::mutex *threadStackMutex = std::get<1>(pointers);
            std::atomic_uint *aCounter = std::get<2>(pointers);
            for (unsigned int i = 0; i < MAXSIZE; ++i) {
                std::thread newThread(
                    [](Internal::ThreadStackType *threadStack,
                       std::mutex *threadStackMutex,
                       Internal::TPQueueType *fnQueue, std::mutex *queueMutex,
                       std::atomic_uint *initCount) {
                        // add id to idStack "call stack"
                        {
                            std::lock_guard<std::mutex> lock(*threadStackMutex);
                            threadStack->push_back(
                                {Internal::ThreadOpt{},
                                 std::this_thread::get_id()});
                        }

                        ++(*initCount);

                        // fetch queued fns and execute them
                        // fnTuples must live until end of function
                        std::list<Internal::TPTupleType> fnTuples;
                        do {
                            bool fnFound = false;
                            {
                                std::lock_guard<std::mutex> lock(*queueMutex);
                                if (!fnQueue->empty()) {
                                    fnTuples.emplace_back(
                                        std::move(fnQueue->front()));
                                    fnQueue->pop();
                                    fnFound = true;
                                }
                            }
                            if (fnFound) {
                                std::get<0>(fnTuples.back())(
                                    std::get<1>(fnTuples.back()));
                            } else {
                                break;
                            }
                        } while (true);

                        // pop id from idStack "call stack"
                        do {
                            std::this_thread::sleep_for(
                                std::chrono::microseconds(15));
                            if (initCount->load() != MAXSIZE) {
                                continue;
                            }
                            {
                                std::lock_guard<std::mutex> lock(
                                    *threadStackMutex);
                                if (std::get<1>(threadStack->back()) ==
                                    std::this_thread::get_id()) {
                                    if (!std::get<0>(threadStack->back())) {
                                        continue;
                                    }
                                    std::get<0>(threadStack->back())->detach();
                                    threadStack->pop_back();
                                    break;
                                }
                            }
                        } while (true);
                    },
                    threadStack, threadStackMutex, &fnQueue, &queueMutex,
                    aCounter);
                // Wait until thread has pushed to threadStack before setting
                // the handle to it
                while (aCounter->load() != i + 1) {
                    std::this_thread::sleep_for(std::chrono::microseconds(15));
                }
                std::lock_guard<std::mutex> stackLock(*threadStackMutex);
                std::get<0>(threadStack->at(i)) = std::move(newThread);
            }
            return pointers;
        } else {
            sequentiallyRunTasks();
        }
        return {nullptr, nullptr, nullptr, nullptr};
    }

    /*!
        \brief Returns true if the function queue is empty.
    */
    bool isQueueEmpty() {
        std::lock_guard<std::mutex> lock(queueMutex);
        return fnQueue.empty();
    }

    /*!
        \brief Returns the MAXSIZE count that this class was created with.
     */
    constexpr unsigned int getMaxThreadCount() { return MAXSIZE; }

    /*!
        \brief Calls startThreads() and waits until all threads have finished.

        Regardless of the value set to MAXSIZE, this function will block until
        all previously queued functions have been executed.
     */
    void easyStartAndWait() {
        if constexpr (MAXSIZE >= 2) {
            Internal::PointersT pointers = startThreads();
            do {
                std::this_thread::sleep_for(std::chrono::microseconds(30));

                bool isQueueEmpty = false;
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    isQueueEmpty = fnQueue.empty();
                }

                if (isQueueEmpty) {
                    break;
                }
            } while (true);
            if (std::get<0>(pointers)) {
                do {
                    {
                        std::lock_guard<std::mutex> lock(
                            *std::get<1>(pointers));
                        if (std::get<0>(pointers)->empty()) {
                            std::get<3>(pointers)->store(false);
                            break;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(15));
                } while (true);
            }
        } else {
            sequentiallyRunTasks();
        }
    }

    /*!
        \brief Checks if any threads are currently running, returning true if
        there are no threads running.
     */
    bool isNotRunning() {
        std::lock_guard<std::mutex> lock(dequesMutex);
        auto tIter = threadStacks.begin();
        auto mIter = threadStackMutexes.begin();
        while (tIter != threadStacks.end() &&
               mIter != threadStackMutexes.end()) {
            {
                std::lock_guard<std::mutex> lock(*mIter);
                if (!tIter->empty()) {
                    return false;
                }
            }
            ++tIter;
            ++mIter;
        }
        return true;
    }

   private:
    Internal::ThreadStacksType threadStacks;
    Internal::ThreadStacksMutexesT threadStackMutexes;
    Internal::TPQueueType fnQueue;
    std::mutex queueMutex;
    Internal::ThreadCountersT threadCounters;
    Internal::PtrsHoldT ptrsHoldBools;
    std::mutex dequesMutex;

    void sequentiallyRunTasks() {
        // pull functions from queue and run them on current thread
        Internal::TPTupleType fnTuple;
        bool hasFn;
        do {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!fnQueue.empty()) {
                    hasFn = true;
                    fnTuple = fnQueue.front();
                    fnQueue.pop();
                } else {
                    hasFn = false;
                }
            }
            if (hasFn) {
                std::get<0>(fnTuple)(std::get<1>(fnTuple));
            }
        } while (hasFn);
    }

    void checkStacks() {
        std::lock_guard<std::mutex> lock(dequesMutex);
        if (threadStacks.empty()) {
            return;
        }

        bool erased = false;
        do {
            erased = false;
            {
                std::lock_guard<std::mutex> lock(threadStackMutexes.front());
                if (ptrsHoldBools.front().load()) {
                    break;
                } else if (threadStacks.front().empty()) {
                    threadStacks.pop_front();
                    threadCounters.pop_front();
                    ptrsHoldBools.pop_front();
                    erased = true;
                }
            }
            if (erased) {
                threadStackMutexes.pop_front();
            } else {
                break;
            }
        } while (!threadStacks.empty() && !threadStackMutexes.empty() &&
                 !threadCounters.empty() && !ptrsHoldBools.empty());
    }

    Internal::PointersT newStackEntry() {
        std::lock_guard<std::mutex> lock(dequesMutex);
        threadStacks.emplace_back();
        threadStackMutexes.emplace_back();
        threadCounters.emplace_back();
        threadCounters.back().store(0);
        ptrsHoldBools.emplace_back();
        ptrsHoldBools.back().store(true);

        return {&threadStacks.back(), &threadStackMutexes.back(),
                &threadCounters.back(), &ptrsHoldBools.back()};
    }
};

}  // namespace EC

#endif
