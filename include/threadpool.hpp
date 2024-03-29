/*
 * @Author: MT
 * @Date: 2024-03-25 09:56:39
 * @FilePath: /src/04_bsw/threadpool/threadpool.hpp
 * @LastEditTime: 2024-03-29 18:28:59
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
#include <atomic>

// #define TP_DEBUG

typedef struct
{
    std::uint32_t core_thread_num;
    std::uint32_t max_thread_num;
    std::uint32_t cur_thread_num;
    std::uint32_t task_queue_num;
    std::uint64_t submit_cnt;
    std::uint64_t execve_cnt;
    std::uint32_t keepalivetime;
    std::uint32_t keepalivecnt;
}tp_info_t;

/**
 * @description: 线程创建模式
 * @Date: 2024-03-27 14:37:03
 * @Author: Motianjie 13571951237@163.com
 */
enum class threadpool_mode : uint8_t
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
 *               2：线程管理:
 *                    a:如果所有线程都在忙碌状态，并且队列中还有新的任务等待处理，线程池创建新的线程来处理这些任务 √
 *                    b:可配置工作线程的启动时机，支持对象创建时启动，支持任务提交时启动
 *               3: 支持返回值获取 √
 * @param numThreads 初始化线程池线程数量
 * @param max_numThreads 线程池线程数量最大值
 * @Date: 2024-03-25 16:07:14
 * @Author: motianjie motianjie@asensing.com
 */
class threadpool 
{
public:
    threadpool(const threadpool& other) = delete;
    threadpool& operator=(const threadpool& other) = delete;
    threadpool(threadpool&& other) = delete;

    threadpool( const std::string& name="", 
                threadpool_mode mode_ = threadpool_mode::immediate,
                std::uint32_t numThreads = std::thread::hardware_concurrency(),
                std::uint32_t max_numThreads = 4 * std::thread::hardware_concurrency(),
                std::uint32_t keepalivetime = 3600
               );
    ~threadpool();

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

    tp_info_t getter_tp_info()const;

private:
    /**
     * @description: 线程工作函数，每个线程单元执行的内容
     * @return {*}
     * @Date: 2024-03-25 16:10:33
     * @Author: motianjie motianjie@asensing.com
     */
    void thread_work_func(int thread_id);

    /**
     * @description: 增加一条工作线程
     * @param {int} thread_id
     * @return {*}
     * @Date: 2024-03-27 16:17:48
     * @Author: Motianjie 13571951237@163.com
     */
    void add_work_thread(int thread_id);

    /**
     * @description: 动态调整线程池的线程数量
     * @return {*}
     * @Date: 2024-03-29 11:17:45
     * @Author: Motianjie 13571951237@163.com
     */    
    bool dyn_adjust();

    /**
     * @description: 定时检查和回收非核心线程
     * @return {*}
     * @Date: 2024-03-29 11:18:09
     * @Author: Motianjie 13571951237@163.com
     */    
    void keepalive_monitor();

    /**
     * @description: 回收空闲线程的资源
     * @return {*}
     * @Date: 2024-03-29 11:23:08
     * @Author: Motianjie 13571951237@163.com
     */    
    void destroyIdleThreads();

    size_t getter_task_queue_size()const;

    size_t getter_thread_size()const;

    /**
     * @description: 初始化创建线程
     * @return {*}
     * @Date: 2024-03-25 16:11:12
     * @Author: motianjie motianjie@asensing.com
     */    
    void start();
    
    /**
     * @description: 启动监控线程
     * @return {*}
     * @Date: 2024-03-29 17:58:45
     * @Author: Motianjie 13571951237@163.com
     */    
    void start_monitor();
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
    std::uint32_t core_thread_num_m;    //soc核心线程数量
    std::uint32_t max_thread_num_m;     //最大线程数量

    std::vector<std::thread> threads;   // need to keep track of threads so we can join them
    std::priority_queue<std::shared_ptr<task_event>,std::vector<std::shared_ptr<task_event>>,priority_cmp> workQueue;   // task_priority_queue
    
    // synchronization
    mutable std::mutex workQueue_mutex;
    std::condition_variable workQueue_condition;
    std::atomic<bool> stopflag;

    mutable std::mutex theads_mutex;
    mutable std::mutex monitor_mutex;
    std::condition_variable monitor_condition;
    std::atomic<bool> start_keepalive_flag;
    std::thread monitor_thread;

    //monitor
    std::atomic<std::uint64_t> submit_cnt_m;
    std::atomic<std::uint64_t> execve_cnt_m;

    //keep alive time unit:seconds
    std::uint32_t keepalivetime_m;
    std::uint32_t keepalive_cnt;

};

inline threadpool::threadpool(const std::string& name,threadpool_mode mode_,std::uint32_t numThreads,std::uint32_t max_numThreads,std::uint32_t keepalivetime) :    name_m(name),
                                                                                                                                        core_thread_num_m(numThreads),
                                                                                                                                        max_thread_num_m(max_numThreads),
                                                                                                                                        stopflag(false),
                                                                                                                                        start_keepalive_flag(false),
                                                                                                                                        submit_cnt_m(0),
                                                                                                                                        execve_cnt_m(0),
                                                                                                                                        keepalivetime_m(keepalivetime),
                                                                                                                                        keepalive_cnt(0)
{
    if(mode_ == threadpool_mode::immediate)  
        start();
    
    start_monitor();
}

inline threadpool::~threadpool()
{
    stop();
#ifdef TP_DEBUG
    SPDLOG_INFO("threadpool::~threadpool()");
#endif
}

inline void threadpool::thread_work_func(int thread_id)
{
    std::string thread_name = "tp_" + name_m + "_" + std::to_string(thread_id);
    pthread_setname_np(pthread_self(),thread_name.c_str());
    while (true) 
    {

        std::shared_ptr<task_event> task;
        {
            std::unique_lock<std::mutex> lock(workQueue_mutex);
            workQueue_condition.wait(lock, [this] { return stopflag.load() || !workQueue.empty(); });
            if (stopflag.load() && workQueue.empty()) 
            {
                return; // 退出线程的循环
            }
            if(!workQueue.empty())
            {
                task = std::move(workQueue.top());
                workQueue.pop();
                execve_cnt_m.fetch_add(1); // execve_cnt++;
#ifdef TP_DEBUG
                int queue_size;
                queue_size = workQueue.size();
                SPDLOG_TRACE("execve task[{}] completed queue size[{}]",execve_cnt_m.load(),queue_size);
#endif
            }
        }
        if(task.get())
        {
            (*task)();
        }
            
    }
}

template<typename F, typename... Args>
inline auto threadpool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of_t<F(Args...)>>
{
    using return_type = typename std::result_of_t<F(Args...)>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...)); //ref count = 1,此处用shared_ptr是因为下面要对task进行拷贝构造，packaged_task不支持拷贝构造，所以用shared_ptr
    auto future = task->get_future(); //获取未来的结果

    auto te = std::make_shared<task_event>([task]() { (*task)(); }); //ref count = 2
    submit_taskevent(te); // 将任务提交给线程池中的线程执行
    return future; //ref count = 1,剩下一个ref count 在队列里，等待任务执行后析构，ref count = 0
}

inline void threadpool::submit_taskevent(std::shared_ptr<task_event> te)
{
    if(stopflag.load())    
        return;
    
    start(); // 延迟启动线程
    {
        std::lock_guard<std::mutex> lock(workQueue_mutex);
        std::lock_guard<std::mutex> lock_(theads_mutex);
        workQueue.push(std::move(te));
        auto ret = dyn_adjust();  //动态调整线程池的数量
        if(ret)
        {
            start_keepalive_flag.store(true);
        }
    }
    submit_cnt_m.fetch_add(1); // sumbit_cnt++;
    workQueue_condition.notify_one();
}

inline void threadpool::add_work_thread(int thread_id)
{
#ifdef TP_DEBUG
    SPDLOG_INFO("thread pool [{}] create thread [{}]",name_m,thread_id);   
#endif 
    threads.emplace_back(std::thread([this,thread_id] { thread_work_func(thread_id); }));
}

inline void threadpool::start_monitor()
{
    monitor_thread = std::thread([this](){ keepalive_monitor();});
}

inline void threadpool::start() 
{
    if(threads.empty())
    {
        for (size_t i = 0; i < core_thread_num_m; ++i) //创建线程
        {
            add_work_thread(i);
        }
    }
}

inline bool threadpool::dyn_adjust()
{
    std::atomic<bool> is_adjust(false);
    auto size = workQueue.size();
    while(size > threads.size() && threads.size() < max_thread_num_m)
    {
        SPDLOG_WARN("thread pool add work thread task queue size:[{}] > threadnum:[{}]",size,threads.size());
        add_work_thread(threads.size());
        is_adjust.store(true);
    }
#ifdef TP_DEBUG
    if(size > max_thread_num_m)
    {
        // SPDLOG_WARN("thread pool [{}] task queue size overflow [{}] maxthreadnum[{}]",name_m,size,max_thread_num_m);
    }
#endif
    return is_adjust.load();
}
inline void threadpool::stop() 
{   
#ifdef TP_DEBUG
    SPDLOG_DEBUG("thread pool ending info:submit_cnt_m[{}] execve_cnt_m[{}]",submit_cnt_m,execve_cnt_m);
#endif
    {
        std::unique_lock<std::mutex> lock(workQueue_mutex);
        std::unique_lock<std::mutex> lock_(monitor_mutex);
        stopflag.store(true);
    }
    workQueue_condition.notify_all();
    monitor_condition.notify_all();
    for (auto& thread : threads) 
    {
        thread.join();
    }
    monitor_thread.join();
}

inline size_t threadpool::getter_task_queue_size()const
{
    std::lock_guard<std::mutex> lock(workQueue_mutex);
    return workQueue.size();
}

inline size_t threadpool::getter_thread_size()const
{
    std::lock_guard<std::mutex> lock(theads_mutex);
    return threads.size();
}

inline void threadpool::keepalive_monitor()
{
    pthread_setname_np(pthread_self(),"tp_keepalive");
    while (1)
    {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        auto ret = monitor_condition.wait_for(lock,std::chrono::seconds(1),[this](){ return stopflag.load();});
        if(ret)
        {
           return;
        }
#ifdef TP_DEBUG
        SPDLOG_INFO("thread size[{}] queue size[{}]",getter_thread_size(),getter_task_queue_size());
#endif
        if(start_keepalive_flag.load())
        {
            //判断是否存在空闲线程,如果有，销毁掉
            if(getter_task_queue_size() < core_thread_num_m)
            {
                keepalive_cnt++;
            }
            if(keepalive_cnt >= keepalivetime_m)
            {
                SPDLOG_WARN("thread pool [{}] adjust destroyidle threads",name_m);
                destroyIdleThreads();
                start_keepalive_flag.store(false);
                keepalive_cnt=0;
            }
        }
    }
    return;
}

inline void threadpool::destroyIdleThreads() 
{
    std::lock_guard<std::mutex> lock(theads_mutex);
    if(threads.size() <= core_thread_num_m)
    {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(workQueue_mutex);
        stopflag.store(true);
    }
    workQueue_condition.notify_all();
    for(auto& thread : threads)
    {
        thread.join();
    }
    threads.clear();
    {
        std::unique_lock<std::mutex> lock(workQueue_mutex);
        stopflag.store(false);
    }

    return;
}

inline tp_info_t threadpool::getter_tp_info()const
{
    tp_info_t tp;
    tp.core_thread_num = core_thread_num_m;
    tp.cur_thread_num = getter_thread_size();
    tp.max_thread_num = max_thread_num_m;
    tp.submit_cnt = submit_cnt_m;
    tp.execve_cnt = execve_cnt_m;
    tp.keepalivecnt = keepalive_cnt;
    tp.keepalivetime = keepalivetime_m;
    tp.task_queue_num = getter_task_queue_size();
    return tp;
}


#endif