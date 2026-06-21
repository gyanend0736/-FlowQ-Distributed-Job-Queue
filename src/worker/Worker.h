#ifndef Worker_h
#define Worker_h

#include<iostream>
#include<string>    
#include<hiredis/hiredis.h>
#include "../queue/redis_queue.h"
#include "../db/postgres.h"


struct Worker
{ 
    redisContext *c;
    JobId workerId;
    Status status;
    JobId current_jobId;
    Worker();
    ~Worker();
    std::bool process(Job& job);
    void handle_faliure(Job& job, R_queue& q, DB& db);
    void run();

};
#endif
