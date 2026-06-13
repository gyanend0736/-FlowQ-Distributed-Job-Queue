#ifndef Worker_h
#define Worker_h
#include "../common/Types.h"
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
    void process(std::string job_id);
    void run(R_queue q);

};
#endif
