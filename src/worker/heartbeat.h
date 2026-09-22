#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <thread>
#include <vector>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<hiredis/hiredis.h>
#include "../common/Job.h"
#include "../queue/redis_connection_pool.h"

class Heartbeat {
public:
    RedisConnectionPool& pool;
    JobId job_id;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread hb_thread;

    // constructor — just store what it needs, don't start yet
    Heartbeat(RedisConnectionPool& pool, JobId job_id) 
        : pool(pool), job_id(job_id), running(false) {}

    // destructor — make sure thread is stopped cleanly
    ~Heartbeat(){
        stop(); 
    }

    void start();
    void stop();
};
#endif