#include <stdio.h>
#include "common_util.h"
#include "key_value_table.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define SHARE_BUFFER_SIZE (1024)

pthread_mutex_t g_cntl_mutex = PTHREAD_MUTEX_INITIALIZER;

void global_mutex_lock()
{
    if (pthread_mutex_lock(&g_cntl_mutex)) {
        LOG_INFO("mutex lock failed!");
    }
}

void global_mutex_unlock()
{
    if (pthread_mutex_unlock(&g_cntl_mutex)) {
        LOG_INFO("mutex unlock failed!");
    }
}

char g_stdin_buf[SHARE_BUFFER_SIZE] = {0};
/* 
    0: pause
    1: running
    2: stop
*/
int cln_flag = 0;

void *global_flag_thread(void *priv)
{
    LOG_INFO("Enter thread!");

    while (1) {
        global_mutex_lock();

        // do something
        if (cln_flag == 1) {
            LOG_INFO("Running...");
        } else if (cln_flag == 0) {
            // doing nothing
        } else if (cln_flag == 2) {
            global_mutex_unlock();
            break;
        }
        global_mutex_unlock();

        sleep(3);
    }
    LOG_INFO("Exit thread!");
    return NULL;
}

sem_t g_cntl_sem;
void *semaphore_control_pthread(void *priv)
{
    pthread_detach(pthread_self());

    LOG_INFO("Enter!");

    while (1) {
        if (sem_wait(&g_cntl_sem) != 0) {
            LOG_INFO("wait semaphore failed!");
            return NULL;
        }

        LOG_INFO("wait first semaphore success!");

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;
        errno = 0;
        int ret = sem_timedwait(&g_cntl_sem, &ts);
        if (errno == ETIMEDOUT) {
            LOG_INFO("wait second semaphore timeout!");
        } else if ( ret == 0) {
            LOG_INFO("wait second semaphore success!");
        } else {
            LOG_INFO("wait second semaphore failed: %s", strerror(errno));
        }
    }


    return NULL;
}

void recur_visit(int n)
{
    if (n < 5) {
        usleep(200 * 1000);
        global_mutex_lock();
        LOG_INFO("recursive deepth: %d, get lock!", n);
        recur_visit(n + 1);
        global_mutex_unlock();
        usleep(200 * 1000);
        LOG_INFO("recursive deepth: %d, release lock!", n);
    }
}

void *recursive_lock_thread(void *priv)
{
    LOG_INFO("Enter thread!");

    recur_visit(0);

    LOG_INFO("Exit thread!");
    return NULL;
}

pthread_cond_t g_cond;

void *condition_wait_thread(void *priv)
{
    LOG_INFO("Enter thread!");

    while (1) {
        global_mutex_lock();
        LOG_INFO("Waiting conditiong!");
        if (pthread_cond_wait(&g_cond, &g_cntl_mutex) != 0) {
            LOG_INFO("wait condition failed: %s", strerror(errno));
            global_mutex_unlock();
            break;
        }
        LOG_INFO("get conditiong success!");

        global_mutex_unlock();
        usleep(200 * 1000);
    }

    LOG_INFO("Exit thread!");
    return NULL;
}

int main(int argc, char const *argv[])
{
    LOG_INFO("Enter main!");
    

    // sem init
    if (sem_init(&g_cntl_sem, 0, 0) != 0) {
        LOG_INFO("Init semaphore failed: %s", strerror(errno));
        return EXCUTE_FAILED_EXIT;
    }

    // mutex init
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_cntl_mutex, &attr)) {
        LOG_INFO("Init mutex lock failed: %s", strerror(errno));
        return EXCUTE_FAILED_EXIT;
    }
    pthread_mutexattr_destroy(&attr);

    pthread_t tid1 = -1;
    pthread_t tid2 = -1;
    pthread_t tid3 = -1;
    pthread_t tid4 = -1;
    if (pthread_create(&tid1, NULL, global_flag_thread, NULL) != 0) {
        LOG_INFO("Create thread failed!");
        return EXCUTE_FAILED_EXIT;
    }

    if (pthread_create(&tid2, NULL, semaphore_control_pthread, NULL) != 0) {
        LOG_INFO("Create thread failed!");
        return EXCUTE_FAILED_EXIT;
    }

    if (pthread_create(&tid3, NULL, recursive_lock_thread, NULL) != 0) {
        LOG_INFO("Create thread failed!");
        return EXCUTE_FAILED_EXIT;
    }

    if (pthread_create(&tid4, NULL, condition_wait_thread, NULL) != 0) {
        LOG_INFO("Create thread failed!");
        return EXCUTE_FAILED_EXIT;
    }

    while (true) {

        if (fgets(g_stdin_buf, SHARE_BUFFER_SIZE, stdin) != NULL) {
            LOG_INFO("get msg: %s", g_stdin_buf);

            global_mutex_lock();
            if (!strncmp(g_stdin_buf, "pause", strlen("pause"))) {
                cln_flag = 0;
            } else if (!strncmp(g_stdin_buf, "run", strlen("run"))) {
                cln_flag = 1;
            } else if (!strncmp(g_stdin_buf, "stop", strlen("stop"))) {
                cln_flag = 2;
            } else if (!strncmp(g_stdin_buf, "sem", strlen("sem"))) {
                if (sem_post(&g_cntl_sem) != 0) {
                    LOG_INFO("Post semaphore failed!");
                }
            } else if (!strncmp(g_stdin_buf, "cond", strlen("cond"))) {
                if (pthread_cond_signal(&g_cond) != 0) {
                    LOG_INFO("Post condition failed!");
                }
            }
            global_mutex_unlock();
        }

        memset(g_stdin_buf, 0, SHARE_BUFFER_SIZE);
        // 0.5S
        usleep(200 * 1000);
    }

    if (sem_destroy(&g_cntl_sem) != 0) {
        LOG_INFO("Destroy semaphore failed!");
        return EXCUTE_FAILED_EXIT;
    }
    return EXCUTE_SUCCESS_EXIT;
}
