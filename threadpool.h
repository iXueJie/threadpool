#pragma once
#include "taskqueue.h"

typedef struct threadpool threadpool;

// 创建并初始化线程池
threadpool *threadpool_create(int capacity, int min, int max);

// 添加任务函数
int threadpool_addTask(threadpool *pool, void *(*f)(void *), void *arg);

// 返回存活的线程数
int threadpool_getLiveNum(threadpool *pool);

// 返回忙的线程数
int threadpool_getBusyNum(threadpool *pool);

// 销毁线程池
void threadpool_destroy(threadpool *pool);