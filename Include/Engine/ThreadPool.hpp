#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <deque>
#include <functional>
#include <future>
#include <thread>
#include <type_traits>
#include <mutex>
#include <condition_variable>
#include <vector>


class ThreadPool
{
public:

    struct Queue
    {
        Queue() :
            mQueue{},
            mLock{},
            mCondVar{},
            mWorkToDo{false} {}

        using Task = std::function<void()>;
        std::vector<Task> mQueue;
        std::mutex mLock;
        std::condition_variable mCondVar;
        bool mWorkToDo;
    };

    ThreadPool(const uint32_t threadCount = std::thread::hardware_concurrency()) :
    mExit(false),
    mLastTaskAddedindex{0},
    mWorkers{},
    mQueues{threadCount}
    {
        for(uint32_t i = 0; i < threadCount; ++i)
        {
            auto workerFunc = [this, i]()
            {
                const uint32_t queueIndex = i;
                while(!mExit)
                {
                    Queue& queue = mQueues[queueIndex];
                    {
                        // Wait for work to be added to the queue.
                        std::unique_lock<std::mutex> lk(queue.mLock);
                        queue.mCondVar.wait(lk, [&](){ return queue.mWorkToDo; });

                        std::vector<Queue::Task> tasks{};
                        queue.mQueue.swap(tasks);
                        queue.mWorkToDo = false;
                        lk.unlock();
                        for(auto& task : tasks)
                            task();
                    }

                }
            };
            mWorkers.emplace_back(workerFunc);
        }
    }

    ~ThreadPool()
    {
        mExit = true;

        for(auto& queue : mQueues)
        {
            std::unique_lock<std::mutex> lock(queue.mLock);
            queue.mWorkToDo = true;
            queue.mCondVar.notify_one();
        }

        for(auto& worker : mWorkers)
            worker.join();
    }

    size_t getWorkerCount() const
    {
        return mWorkers.size();
    }

    template<typename F, typename ...Args>
    auto addTask(F&& f, Args&& ...a) -> std::future<typename std::result_of_t<F(Args...)>>
    {
        using return_type = std::result_of_t<F(Args...)>;
        std::packaged_task<return_type()>* task = new std::packaged_task<return_type()>{std::bind(std::forward<F>(f), std::forward<Args>(a)...)};

        std::future<return_type> future = task->get_future();

        auto work_unit = [task]() mutable
        {
            (*task)();
            delete task;
        };
        Queue& queue = mQueues[mLastTaskAddedindex];
        mLastTaskAddedindex = (mLastTaskAddedindex + 1) % mWorkers.size();

        std::unique_lock<std::mutex> lock(queue.mLock);
        queue.mQueue.push_back(std::move(work_unit));
        queue.mWorkToDo = true;
        queue.mCondVar.notify_one();

        return future;
    }

private:

    bool mExit;
    uint32_t mLastTaskAddedindex;

    std::vector<std::thread> mWorkers;

    std::vector<Queue> mQueues;

};


#endif
