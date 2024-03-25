/*
 * @Author: MT
 * @Date: 2024-03-25 17:36:47
 * @FilePath: /threadpool/main.cpp
 * @LastEditTime: 2024-03-25 17:59:26
 * @LastEditors: MT
 * @copyright: asensing.co
 */
#include "include/threadpool.hpp"
#include <iostream>
#include <stdlib.h>
int add(int a, int b) 
{
    return a + b;
    std::cout << a << " + " << b << " = " << a + b << std::endl;
}

void test_0()
{
    printf("Testing 0\n");
}
void test_1()
{
    printf("Testing 1\n");
}

void test_2()
{
    printf("Testing 2\n");
}
void test_3()
{
    printf("Testing 3\n");
}
void test_4()
{
    printf("Testing 4\n");
}
void test_5()
{
    printf("Testing 5\n");
}
void test_6()
{
    printf("Testing 6\n");
}
void test_7()
{
    printf("Testing 7\n");
}
void test_8()
{
    printf("Testing 8\n");
}
void test_9()
{
    printf("Testing 9\n");
}



int main() 
{
    //***********test priority***********//
    // auto task_0 = std::make_shared<task_event>(test_0,0);
    // auto task_1 = std::make_shared<task_event>(test_1,1);
    // auto task_2 = std::make_shared<task_event>(test_2,2);
    // auto task_3 = std::make_shared<task_event>(test_3,3);
    // auto task_4 = std::make_shared<task_event>(test_4,4);
    // auto task_5 = std::make_shared<task_event>(test_5,5);
    // auto task_6 = std::make_shared<task_event>(test_6,6);
    // auto task_7 = std::make_shared<task_event>(test_7,7);
    // auto task_8 = std::make_shared<task_event>(test_8,8);
    // auto task_9 = std::make_shared<task_event>(test_9,9);

    // ThreadPool pool(20,60);
    // pool.submit_taskevent(task_0);
    // pool.submit_taskevent(task_1);
    // pool.submit_taskevent(task_2);
    // pool.submit_taskevent(task_3);
    // pool.submit_taskevent(task_4);
    // pool.submit_taskevent(task_5);
    // pool.submit_taskevent(task_6);
    // pool.submit_taskevent(task_7);
    // pool.submit_taskevent(task_8);
    // pool.submit_taskevent(task_9);
    //***********test priority***********//

    //***********get return value test***********//
    // ThreadPool pool;
    // for (int i = 0; i < 1000; ++i) {
    //     auto result = pool.submit(add, i, i + 1);
    //     // 获取任务的返回值
    //     std::cout << "result is [" << result.get() << "]\n" << std::endl;
    // }
    //***********get return value test***********//
    return 0;
}