#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "taskqueue.h"

// 队列的常用便捷操作
#define next_position(queue, side) ((queue->side + 1) % queue->capacity)      // 返回下一个位置
#define next(queue, side) queue->side = (queue->side + 1) % queue->capacity    // 指向下一个位置

// 创建任务对列并初始化
taskqueue *taskqueue_create(size_t capacity)
{
    taskqueue *taskq = (taskqueue *)malloc(sizeof(taskqueue));
    
    // 初始化队列
    do
    {
        if (taskq == NULL)
        {
            printf("threadpool.taskqueue allocation failed.\n");
            break;
        }

        taskq->tasks = (threadpool_task *)malloc(sizeof(threadpool_task));
        if (taskq->tasks == NULL)
        {
            printf("threadpool.taskqueue.tasks allocation failed.\n");
            break;
        }

        taskq->capacity = capacity;
        taskq->front = taskq->rear = taskq->len = 0;

        return taskq;
    } while (0);

    taskqueue_destroy(taskq);
    return NULL;
}

// 添加任务
int taskqueue_enqueue(taskqueue* taskq, const threadpool_task *task)
{
    if (taskq->len < taskq->capacity)
    {
        taskq->tasks[taskq->rear].function = task->function;
        taskq->tasks[taskq->rear].arg = task->arg;
        ++(taskq->len);
        next(taskq, rear);
        return 1;
    }
    else
    {
        return 0;
    }
}

// 取出任务
int taskqueue_dequeue(taskqueue* taskq, threadpool_task *ret_task)
{
    if (taskq->len > 0)
    {
        ret_task->function = taskq->tasks[taskq->front].function;
        ret_task->arg = taskq->tasks[taskq->front].arg;
        --(taskq->len);
        next(taskq, front);
        return 1;
    }
    else
    {
        return 0;
    }
}

// 销毁任务对列
void *taskqueue_destroy(taskqueue* taskq)
{
    free(taskq->tasks);
    taskq->tasks = NULL;
    free(taskq);
}