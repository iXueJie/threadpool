#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "threadpool.h"
#include "taskqueue.h"

#define ADD_BATCH 2

typedef struct threadpool
{
    // 任务队列
    taskqueue *tasks;

    // 线程
    pthread_t admin_tid;        // 管理者线程
    pthread_t *thread_IDs;  // 线程数组
    int max;        // 最大线程数
    int min;        // 最小线程数
    int livenum;    // 存活线程数
    int busynum;    // 忙线程数
    int exit;       // 待销毁线程数

    pthread_mutex_t mutex_pool;
    pthread_mutex_t mutex_busynum;
    pthread_cond_t notFull;
    pthread_cond_t notEmpty;

    bool shutdown;
} threadpool;

// 创建并初始化线程池
threadpool *threadpool_create(int capacity, int min, int max)
{
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));
    // 按照结构体声明顺序初始化变量
    do
    {
        if (pool == NULL)
        {
            printf("threadpoll allocation failed.\n");
            break;
        }
        pool->tasks = taskqueue_create(capacity);
        if (pool->tasks == NULL)
        {
            printf("threadpoll.tasks creation failed.\n");
            break;
        }

        // TODO: 创建线程失败，可以打印具体的失败信息
        // 创建线程
        size_t tIDs_size = sizeof(pthread_t) * max;
        pool->thread_IDs = (pthread_t *)malloc(tIDs_size);
        memset(pool->thread_IDs, 0, tIDs_size);     // 默认休眠线程tID为0，忙线程非0

        // 管理者线程
        if (pthread_create(&pool->admin_tid, NULL, admin, pool) != 1)
        {
            printf("threadpoll.admin_thread creation failed.\n");
            break;
        }

        // 工作线程
        bool t_init = false;
        for (size_t i = 0; i < min; i++)
        {
            if (pthread_create(&pool->admin_tid, NULL, admin, pool) != 1)
            {
                printf("threadpoll.threads[%d] creation failed.\n", i);
                t_init = true;
                break;
            }
            
        }
        if (t_init)
        {
            break;
        }
        pool->max = max;
        pool->min = min;
        pool->livenum = min;
        pool->exit = 0;

        // 互斥锁与条件变量
        if (pthread_mutex_init(&pool->mutex_pool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutex_pool, NULL) != 0)
        {
            printf("threadpoll.mutex initialization failed.\n");
            break;
        }
        if (pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 )
        {
            printf("threadpoll.cond initialization failed.\n");
            break;
        }

        pool->shutdown = false;
        return pool;
    } while (0);
    
    threadpool_free(pool);
    return NULL;
}

// TODO: 出现重大问题！！
//       队列与条件变量冲突！！
// 添加任务函数
int threadpool_addTask(threadpool *pool, void *(*function)(void *), void *arg)
{
    int status; // 是否执行成功
    if (!pool->shutdown)
    {
        pthread_mutex_lock(&pool->mutex_pool);
        status = taskqueue_enqueue(&pool->tasks, function, arg);
        pthread_mutex_unlock(&pool->mutex_pool);
    }
    return status;
}

// 返回存活的线程数
int threadpool_getLiveNum(threadpool *pool)
{
    pthread_mutex_lock(&pool->mutex_pool);
    int livenum = pool->livenum;
    pthread_mutex_unlock(&pool->mutex_pool);
    return livenum;
}

// 返回忙的线程数
int threadpool_getBusyNum(threadpool *pool)
{
    pthread_mutex_lock(&pool->mutex_busynum);
    int busynum = pool->busynum;
    pthread_mutex_unlock(&pool->mutex_busynum);
    return busynum;
}

// 销毁线程池
void threadpool_destroy(threadpool *pool);

void threadpool_free(threadpool *pool);

// 管理者函数
void *admin(void* arg)
{
    threadpool *pool = (threadpool *)arg;
    int counter = 0;
    while (!pool->shutdown)
    {
        // 每个3s检测一次
        sleep(3);

        // 去除线程池中任务的数量和当前现成的数量
        pthread_mutex_lock(&pool->mutex_pool);
        int qlen = taskqueue_len(&pool->tasks);
        int liveNum = pool->livenum;
        pthread_mutex_unlock(&pool->mutex_pool);

        // 取出忙线程数量
        pthread_mutex_lock(&pool->mutex_busynum);
        int busyNum = pool->busynum;     
        pthread_mutex_unlock(&pool->mutex_busynum);

        // 添加线程
        // 任务的个数>存活的线程数 && 存活的线程数<最大线程数
        if (qlen > liveNum && liveNum < pool->max)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            for (int i = 0; i < pool->max && counter < ADD_BATCH && pool->livenum < pool->max; i++)
            {
                if (pool->thread_IDs[i] == 0)
                {
                    pthread_create(&pool->thread_IDs[i], NULL, worker, pool);
                    counter++;
                    pool->livenum++;
                }
            }
            pthread_mutex_unlock(&pool->mutex_pool);
        }

        // 销毁线程
        // 忙线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->min)
        {
            pthread_mutex_lock(&pool->mutex_pool);
            pool->exit = ADD_BATCH;
            pthread_mutex_unlock(&pool->mutex_pool);
            for (int  i = 0; i < ADD_BATCH; i++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    
    return NULL;
}

// 工作函数
void *worker(void* arg)
{
    threadpool *pool = (threadpool *)arg;
    threadpool_task task;
    task.function = NULL;
    task.arg = NULL;

    while (1)
    {
        pthread_mutex_lock(&pool->mutex_pool);
        while (taskqueue_len(&pool->tasks) == 0 && !pool->shutdown)
        {
            // 阻塞线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutex_pool);
            // 线程退出
            if (pool->exit > 0)
            {
                pool->exit--;
                if (pool->livenum > pool->min)
                {
                    // TODO: 可以优化，有一个变量专门记录位置
                    pthread_t tid = pthread_self();
                    for (int i = 0; i < pool->max; i++)
                    {
                        if (pool->thread_IDs[i] == tid)
                        {
                            pool->thread_IDs[i] = 0;
                            pthread_mutex_unlock(&pool->mutex_pool);
                            break;
                        }
                    }
                    pthread_exit(NULL);
                }
            }
        }

        // 判断线程池是否关闭
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutex_pool);
            pthread_exit(NULL);
        }

        // 从任务队列中取出任务
        taskqueue_dequeue(&pool->tasks, &task);
        // 解锁线程池
        pthread_mutex_unlock(&pool->mutex_pool);

        // 执行任务，忙线程数++
        pthread_mutex_lock(&pool->busynum);
        pool->busynum++;
        pthread_mutex_unlock(&pool->busynum);

        printf("Thread 0x%x start working ...\n", pthread_self());
        task.function(task.arg);
        printf("Thread 0x%x end working ...\n", pthread_self());
        
        // 执行完毕，忙线程数--
        pthread_mutex_lock(&pool->busynum);
        pool->busynum--;
        pthread_mutex_unlock(&pool->busynum);
        
        free(arg);
    }
    
    return NULL;
}
