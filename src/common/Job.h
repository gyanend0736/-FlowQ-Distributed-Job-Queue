#ifndef Job_h
#define Job_h

#include "Types.h"
#include<iostream>
#include<string>

struct Job
{
    JobId job_id;
    std::string idempotent_key;
    std::string type;
    json payload;
    Priority priority;
    WaitTime run_at;
    Status status;
    int attempts;
    int max_retries;
    int next_retry_at;
    Time created_at;
    Time started_at;
    Time completed_at;
    json result;
    json error;

   

};


#endif