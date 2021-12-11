#pragma once
#include <pthread.h>
#include <stdbool.h>
#include "taskqueue.h"

typedef struct threadpool
{
    // 任务队列
    taskqueue *tasks;

    // 线程数组
    pthread_t *thread_IDs;
    int max;        // 最大线程数
    int min;        // 最小线程数
    int livenum;    // 存活线程数
    int busynum;    // 忙线程数
    int exit;       // 待销毁线程数

    pthread_t admin;

    bool shutdown;
} threadpool;


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