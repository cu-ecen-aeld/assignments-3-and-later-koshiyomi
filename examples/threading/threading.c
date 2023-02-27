#include "threading.h"
#include <unistd.h>
#include <stdlib.h>

#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

int msleep(int millisecond){
     return usleep(millisecond * 1000);
}

void* threadfunc(void* thread_param)
{
    struct thread_data *threadData = (struct thread_data *)thread_param;

    bool status = true;
    status = status && msleep(threadData->wait_to_obtain_ms) == 0;
    status = status && pthread_mutex_lock(threadData->mutex) == 0;
    status = status && msleep(threadData->wait_to_release_ms) == 0;
    status = status && pthread_mutex_unlock(threadData->mutex) == 0;

    threadData->thread_complete_success = status;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *thread_data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (thread_data == NULL) {
        return false;
    }

    thread_data->thread = thread;
    thread_data->mutex = mutex;
    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;

    return pthread_create(thread, NULL, threadfunc, thread_data) == 0;
}

