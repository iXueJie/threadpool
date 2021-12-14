#pragma once
#include <stdbool.h>

typedef struct threadpool_task
{
    void *(*function)(void *);
    void *arg;
} threadpool_task;

typedef struct taskqueue
{
    threadpool_task *tasks;
    int capacity;
    int front;
    int rear;
    int len;
} taskqueue;

// 创建任务对列并初始化
taskqueue *taskqueue_create(size_t capacity);

// 添加任务
int taskqueue_enqueue(taskqueue* taskq, void *(*function)(void *), void *arg);

// 取出任务
int taskqueue_dequeue(taskqueue* taskq, threadpool_task *ret_task);

// 获取队列长度
int taskqueue_len(const taskqueue *taskq);

// 判断队列是否满了
bool taskqueue_isfull(const taskqueue *taskq);

// 销毁任务对列
void *taskqueue_destroy(taskqueue* taskq);