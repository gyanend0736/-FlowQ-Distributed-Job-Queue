#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <thread>
#include <vector>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<hiredis/hiredis.h>
#include "../common/Job.h"

class Heartbeat {
public:
    redisContext* c;
    JobId job_id;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread hb_thread;

    // constructor — just store what it needs, don't start yet
    Heartbeat(redisContext* c, JobId job_id) 
        : c(c), job_id(job_id), running(false) {}

    // destructor — make sure thread is stopped cleanly
    ~Heartbeat(){
        stop(); 
    }

    void start();
    void stop();
};
#endif