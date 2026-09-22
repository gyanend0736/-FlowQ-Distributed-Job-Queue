#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <hiredis/hiredis.h>
#include "../queue/redis_queue.h"
#include "../queue/redis_connection_pool.h"

class Scheduler {
public:
    std::unique_ptr<RedisConnectionPool> pool;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread sched_thread;
    R_queue q;
    

    Scheduler();
    ~Scheduler(){};
    

    void start();
    void stop();

private:
    void tick(); // one pass: find due jobs, move them to ready
};
#endif