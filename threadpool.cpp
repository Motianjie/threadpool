#include <iostream>
#include <thread>
#include <functional>
#include <future>
#include <vector>
#include <queue>
// 定义一个线程池类
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : threads(numThreads),
                                    stopflag(false)
     {}
    
    ~ThreadPool()
    {
        stop();
    }

    
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) 
    {
        // 使用 std::packaged_task 将任务包装起来
        std::packaged_task<std::result_of_t<F(Args...)>()> task(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto future = task.get_future();
        
        // 将任务提交给线程池中的线程执行
        std::unique_lock<std::mutex> lock(mutex);
        workQueue.push(std::move(task));
        condition.notify_one();
        return future;
    }
    
    void start() {
        for (size_t i = 0; i < threads.size(); ++i) {
            threads[i] = std::thread([this] 
            {
                while (true) 
                {
                    std::packaged_task<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock, [this] { return stopflag || !workQueue.empty(); });
                        if (stopflag && workQueue.empty()) 
                        {
                            return; // 退出线程的循环
                        }
                        task = std::move(workQueue.front());
                        workQueue.pop();
                    }
                    task();
                }
            });
        }
    }
    
    void stop() 
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stopflag = true;
        }
        condition.notify_all();

        for (auto& thread : threads) 
        {
            thread.join();
        }
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::packaged_task<void()>> workQueue;
    std::mutex mutex;
    std::condition_variable condition;
    bool stopflag;
};

// 示例任务函数
void add(int a, int b) 
{
    std::cout << a << " + " << b << " = " << a + b << std::endl;
}

int main() {
    ThreadPool pool(4);
    pool.start();

    for (int i = 0; i < 10; ++i) {
        auto result = pool.submit(add, i, i + 1);
        // 获取任务的返回值
        result.get();
    }

    return 0;
}