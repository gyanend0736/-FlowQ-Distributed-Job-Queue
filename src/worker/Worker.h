#ifndef Worker_h
#define Worker_h

#include<iostream>
#include<string>    
#include<hiredis/hiredis.h>
#include "../queue/redis_queue.h"
#include "../db/postgres.h"
#include <atomic>

struct Worker
{ 
    redisContext *c;
    JobId workerId;
    Status status;
    JobId current_jobId;
    Worker();
    ~Worker();
    bool process(Job& job);
    void handle_faliure(Job& job, R_queue& q, DB& db);
    void run(std:: atomic<bool>& running);

};
#endif
