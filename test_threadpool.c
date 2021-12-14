#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "threadpool.h"

#include "utils.h"

void *taskFunc(void* arg)
{
    int num = *(int*)arg;
    printf("thread %ld is working, number = %d\n",
        pthread_self(), num);
    sleep(1);
    return NULL;
}

int main()
{
    // 创建线程池
    threadpool* pool = threadpool_create(100, 2, 6);
    do
    {
        if (pool == NULL)
        {
            break;
        }
        for (int i = 0; i < 100; ++i)
        {   
            int* num = (int*)malloc(sizeof(int));
            *num = i + 100;
            threadpool_addTask(pool, taskFunc, num);
        }
        debug;
        sleep(30);
        debug;
        threadpool_destroy(pool);    
    } while (0);
    
    return 0;
}
