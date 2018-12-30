#include <queue>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define JOB_CHECK_DELAY 100

using namespace std;

class ThreadPool;
void* threadLoopHelper(void* args);
typedef void job_funct_t(void*);

typedef struct {
    void (*f)(void*);
    void* params;
} job_t;

typedef struct {
    int tid;
    ThreadPool* pool;
} loop_helper_args_t;


class ThreadPool
{
private:
    /* data */
    int thread_count;
    bool join_threads = false;

    vector<pthread_t> threads;

    pthread_mutex_t queue_mutex;

    queue<job_t> job_queue;

    struct timespec tim;
   
public:
    ThreadPool(int);
    ~ThreadPool();
    void addJob(job_funct_t, void*);
    void fetchJob(int);
    bool jobsRemaining();
    void joinThreads();
    void threadLoop(int);
};

ThreadPool::ThreadPool(int thread_count)
{
    this->thread_count = thread_count;
    this->queue_mutex = PTHREAD_MUTEX_INITIALIZER;

    this->tim.tv_sec = 0;
    this->tim.tv_nsec = JOB_CHECK_DELAY;

    this->threads.resize(thread_count);
    this->job_queue = queue<job_t>();
    
    for (int i = 0; i < thread_count; i++) {
        loop_helper_args_t* lh_args = new loop_helper_args_t;
        lh_args->tid = i;
        lh_args->pool = this;

        pthread_create(&this->threads[i], NULL, threadLoopHelper, lh_args);
    }
}

void ThreadPool::addJob(void (*f)(void*), void* params) {
    job_t job;
    job.f = f;
    job.params = params;

    pthread_mutex_lock(&this->queue_mutex);

    this->job_queue.push(job);

    pthread_mutex_unlock(&this->queue_mutex);
}

void ThreadPool::fetchJob(int tid) {

    pthread_mutex_lock(&this->queue_mutex);
    
    job_t job;
    bool job_fetched = false;
    
    if (!this->job_queue.empty()) {
        job = this->job_queue.front();
        this->job_queue.pop();
        job_fetched = true;
    }

    pthread_mutex_unlock(&this->queue_mutex);

    if (job_fetched) {
        job.f(job.params);
    }
}

bool ThreadPool::jobsRemaining() {

    pthread_mutex_lock(&this->queue_mutex);
    
    bool remaining = !this->job_queue.empty();

    pthread_mutex_unlock(&this->queue_mutex);

    return remaining;
}

void ThreadPool::joinThreads() {
    this->join_threads = true;

    for (int i = 0; i < this->thread_count; i++) {
        pthread_join(this->threads[i], NULL);
    }
}

void ThreadPool::threadLoop(int tid) {
    while (!this->join_threads) {
        this->fetchJob(tid);
        nanosleep(&this->tim, NULL);
    }
}

ThreadPool::~ThreadPool()
{
}

void* threadLoopHelper(void* args) {
    loop_helper_args_t* lh_args = (loop_helper_args_t*) args;

    lh_args->pool->threadLoop(lh_args->tid);
}
