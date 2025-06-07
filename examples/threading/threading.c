#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    // wait to obtain

    // Cast void pointer to thread_data type
    struct thread_data *data = (struct thread_data *)thread_param;

    struct timespec ts;
    ts.tv_sec = data->wait_to_obtain_ms / 1000;
    ts.tv_nsec = (data->wait_to_obtain_ms % 1000) * 1000000;
    nanosleep(&ts, NULL);

    DEBUG_LOG("locking mutex");
    // obtain mutex
    int ret = pthread_mutex_lock(data->mutex);
    DEBUG_LOG("locking mutex ret %d", ret);
    if (ret != 0)
    {
        ERROR_LOG("Failed to lock mutex: %d", ret);
        data->thread_complete_success = false;
        return NULL;
    }

    // wait to release
    ts.tv_sec = data->wait_to_release_ms / 1000;
    ts.tv_nsec = (data->wait_to_release_ms % 1000) * 1000000;
    nanosleep(&ts, NULL);

    // release lock
    DEBUG_LOG("releasing mutex");
    ret = pthread_mutex_unlock(data->mutex);
    DEBUG_LOG("releasing mutex ret %d", ret);
    if (ret != 0)
    {
        ERROR_LOG("Failed to unlock mutex: %d", ret);
        data->thread_complete_success = false;
        return NULL;
    }

    data->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    // Allocate memory for thread_data
    DEBUG_LOG("\n ----------------- \n");
    struct thread_data *tdata = malloc(sizeof(struct thread_data));
    if (tdata == NULL)
    {
        ERROR_LOG("Failed to allocate tdata");
        return false;
    }
    // setup mutex
    tdata->mutex = mutex;
    // setup wait
    tdata->wait_to_obtain_ms = wait_to_obtain_ms;
    tdata->wait_to_release_ms = wait_to_release_ms;
    tdata->thread_complete_success = false;
    DEBUG_LOG("\n\n -- Creating thread -- \n\n");
    // Create thread running threadfunc
    int ret = pthread_create( thread, NULL, threadfunc, tdata );
    DEBUG_LOG("Created thread: %d", ret);
    if (ret != 0)
    {
        ERROR_LOG("Thread creation failed with error %d", ret);
        free(tdata);
        return false; 
    }

    DEBUG_LOG("\n ----------------- \n");
    return true;
}

