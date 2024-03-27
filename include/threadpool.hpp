/*
 * @Author: MT
 * @Date: 2024-03-25 09:56:39
 * @FilePath: /threadpool_soam/threadpool/threadpool.hpp
 * @LastEditTime: 2024-03-27 14:48:06
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
#include <iostream>
#include "spdlog/spdlog.h"

/**
 * @description: 线程创建模式
 * @Date: 2024-03-27 14:37:03
 * @Author: Motianjie 13571951237@163.com
 */
enum class mode : uint8_t
{
    latency = 0x00, //延迟创建模式，对象创建时不创建线程，等到有任务注入时创建
    immediate = 0x01,//立即创建模式，对象创建时立即创建线程
};


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

    ThreadPool(const std::string& name="", mode mode_ = mode::immediate,std::uint32_t numThreads = std::thread::hardware_concurrency(),std::uint32_t max_numThreads = 4 * std::thread::hardware_concurrency());
    ~ThreadPool();

public:
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
    void thread_work_func(int thread_id);

    void add_work_thread(int thread_id);

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
    std::string name_m;
    std::vector<std::thread> work_threads_m; //工作线程池
    std::uint32_t current_thread_num_m; //当前线程数量
    std::uint32_t max_thread_num_m;     //最大线程数量

    std::vector<std::thread> threads;   // need to keep track of threads so we can join them
    std::priority_queue<std::shared_ptr<task_event>,std::vector<std::shared_ptr<task_event>>,priority_cmp> workQueue;   // task_priority_queue
    
    // synchronization
    std::mutex mutex;
    std::condition_variable condition;
    std::atomic<bool> stopflag;

};

inline ThreadPool::ThreadPool(const std::string& name,mode mode_,std::uint32_t numThreads,std::uint32_t max_numThreads) :   name_m(name),
                                                                                                                            current_thread_num_m(numThreads),
                                                                                                                            max_thread_num_m(max_numThreads),
                                                                                                                            stopflag(false)
{
    if(mode_ == mode::immediate)  
        start();
}

inline ThreadPool::~ThreadPool()
{
    stop();
    std::cout << "ThreadPool::~ThreadPool() " << std::endl;
}

inline void ThreadPool::thread_work_func(int thread_id)
{
    std::string thread_name = "tp_" + name_m + "_" + std::to_string(thread_id);
    pthread_setname_np(pthread_self(),thread_name.c_str());
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
            if(!workQueue.empty())
            {
                task = std::move(workQueue.top());
                workQueue.pop();
            }
        }
        if(task.get())
        {
            (*task)();
        }
            
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
    
    start();
    std::lock_guard<std::mutex> lock(mutex);
    workQueue.push(std::move(te));
    auto size = workQueue.size();
    while(size > current_thread_num_m && current_thread_num_m < max_thread_num_m)
    {
        SPDLOG_WARN("thread pool add work thread task queue size:[{}] > threadnum:[{}]",size,current_thread_num_m);
        add_work_thread(current_thread_num_m);
        current_thread_num_m = threads.size();
    }
    if(size > max_thread_num_m)
    {
        SPDLOG_WARN("thread pool [{}] task queue size overflow [{}] maxthreadnum[{}]",name_m,size,max_thread_num_m);
    }
    condition.notify_one();
}

inline void ThreadPool::add_work_thread(int thread_id)
{
    SPDLOG_INFO("thread pool [{}] create thread [{}]",name_m,thread_id);    
    threads.emplace_back(std::thread([this,thread_id] { thread_work_func(thread_id); }));
}

inline void ThreadPool::start() 
{
    if(threads.empty())
    {
        for (size_t i = 0; i < current_thread_num_m; ++i) //创建线程
        {
            add_work_thread(i);
        }
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