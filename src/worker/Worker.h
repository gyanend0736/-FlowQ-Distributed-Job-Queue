#ifndef Worker_h
#define Worker_h

#include<iostream>
#include<string>    
#include<hiredis/hiredis.h>
#include "../queue/redis_queue.h"


struct Worker
{ 
    redisContext *c;
    JobId workerId;
    Status status;
    JobId current_jobId;
    Worker();
    ~Worker();
    std::bool process(Job& job);
    void handle_faliure(Job& job);
    void run(R_queue& q);

};
#endif
