/*
 * @Author: MT
 * @Date: 2024-03-25 09:56:39
 * @FilePath: /threadpool/include/threadpool.hpp
 * @LastEditTime: 2024-03-25 17:58:34
 * @LastEditors: MT
 * @copyright: asensing.co
 */
#ifndef __THREADPOOL__
#define __THREADPOOL__

#include <thread>
#include <functional>
#include <future>
#include <vector>
#include <queue>

/**
 * @description: 任务类，优先级数值越小，优先级越高,优先级为0~255
 * @Date: 2024-03-25 15:27:01
 * @Author: motianjie motianjie@asensing.com
 */
class task_event
{
public:
    using priority = std::uint8_t;
    task_event()=delete;
    
    task_event(std::function<void()> func,priority priority_ = 0u) : func_m(func),
                                                                     priority_m(priority_)
    {

    }

    ~task_event()
    {

    }

    const priority getter_priority()const noexcept
    {
        return priority_m;
    }

    void operator()()
    {
        func_m();
    }
private:
    std::function<void()> func_m{nullptr};
    priority priority_m;
};

/**
 * @description: 自定义优先队列的排序，小顶堆，数字越小优先级越高
 * @Date: 2024-03-25 16:16:24
 * @Author: motianjie motianjie@asensing.com
 */
struct priority_cmp
{
    bool operator()(const std::shared_ptr<task_event>& task_left,const std::shared_ptr<task_event>&  task_right) const
    {
        return task_left->getter_priority() > task_right->getter_priority();
    }
};

/**
 * @description: 线程池类
 *               1: 支持优先级设定，优先级越高，线程池优先处理 √
 *               2：线程管理：如果所有线程都在忙碌状态，并且队列中还有新的任务等待处理，线程池创建新的线程来处理这些任务 √
 *               3: 支持返回值获取 √
 * @param numThreads 初始化线程池线程数量
 * @para max_numThreads 线程池线程数量最大值
 * @Date: 2024-03-25 16:07:14
 * @Author: motianjie motianjie@asensing.com
 */
class ThreadPool 
{
public:
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;
    ThreadPool(ThreadPool&& other) = delete;

    ThreadPool(std::uint32_t numThreads = std::thread::hardware_concurrency(),std::uint32_t max_numThreads = 4 * std::thread::hardware_concurrency());
    ~ThreadPool();

    /**
     * @description: 提交任务至线程池
     * @param {F&&}
     * @param {Args&&...}
     * @return {std::future<typename std::result_of_t<F(Args...)>>}
     * @Date: 2024-03-25 15:28:23
     * @Author: motianjie motianjie@asensing.com
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of_t<F(Args...)>>;

    /**
     * @description: 提交任务至线程池
     * @param {shared_ptr<task_event>} te
     * @return {*}
     * @Date: 2024-03-25 15:28:06
     * @Author: motianjie motianjie@asensing.com
     */    
    void submit_taskevent(std::shared_ptr<task_event> te);

private:
    /**
     * @description: 线程工作函数，每个线程单元执行的内容
     * @return {*}
     * @Date: 2024-03-25 16:10:33
     * @Author: motianjie motianjie@asensing.com
     */
    void thread_work_func();

    /**
     * @description: 初始化创建线程
     * @return {*}
     * @Date: 2024-03-25 16:11:12
     * @Author: motianjie motianjie@asensing.com
     */    
    void start();
    
    /**
     * @description: 停止所有工作线程，并回收资源
     * @return {*}
     * @Date: 2024-03-25 16:11:32
     * @Author: motianjie motianjie@asensing.com
     */    
    void stop();

private:
    std::uint32_t current_thread_num_m; //当前线程数量
    std::uint32_t max_thread_num_m;     //最大线程数量

    std::vector<std::thread> threads;   // need to keep track of threads so we can join them
    std::priority_queue<std::shared_ptr<task_event>,std::vector<std::shared_ptr<task_event>>,priority_cmp> workQueue;   // task_priority_queue
    
    // synchronization
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> stopflag;
};

inline ThreadPool::ThreadPool(std::uint32_t numThreads,std::uint32_t max_numThreads) :  current_thread_num_m(numThreads),
                                                                                        max_thread_num_m(max_numThreads),
                                                                                        stopflag(false)
{
    start();
}

inline ThreadPool::~ThreadPool()
{
    stop();
}

inline void ThreadPool::thread_work_func()
{
    while (true) 
    {
        std::shared_ptr<task_event> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this] { return stopflag.load() || !workQueue.empty(); });
            if (stopflag.load() && workQueue.empty()) 
            {
                return; // 退出线程的循环
            }
            task = std::move(workQueue.top());
            workQueue.pop();
        }
        (*task)();
    }
}

template<typename F, typename... Args>
inline auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of_t<F(Args...)>>
{
    using return_type = typename std::result_of_t<F(Args...)>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...)); //ref count = 1,此处用shared_ptr是因为下面要对task进行拷贝构造，packaged_task不支持拷贝构造，所以用shared_ptr
    auto future = task->get_future(); //获取未来的结果

    auto te = std::make_shared<task_event>([task]() { (*task)(); }); //ref count = 2
    submit_taskevent(te); // 将任务提交给线程池中的线程执行
    return future; //ref count = 1,剩下一个ref count 在队列里，等待任务执行后析构，ref count = 0
}

inline void ThreadPool::submit_taskevent(std::shared_ptr<task_event> te)
{
    if(stopflag.load())    
        return;
    
    std::lock_guard<std::mutex> lock(mutex);
    workQueue.push(std::move(te));
    auto size = workQueue.size();
    if(size > current_thread_num_m && size <= max_thread_num_m) //如果任务队列大小比线程池线程数量大，证明线程池处理不过来，需要新增线程处理
    {
        while(size > current_thread_num_m)
        {
            threads.emplace_back(std::thread([this] { thread_work_func(); }));
            current_thread_num_m = threads.size();
        }
    }   
    condition.notify_one();
}

inline void ThreadPool::start() 
{
    for (size_t i = 0; i < current_thread_num_m; ++i) //创建线程
    {
        threads.emplace_back(std::thread([this] { thread_work_func(); }));
    }
}

inline void ThreadPool::stop() 
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        stopflag.store(true);
    }
    condition.notify_all();

    for (auto& thread : threads) 
    {
        thread.join();
    }
}

#endif