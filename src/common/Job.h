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
NLOHMANN_JSON_SERIALIZE_ENUM(Status, {
    {Status::PENDING, "pending"},
    {Status::PROCESSING, "processing"},
    {Status::DONE, "done"},
    {Status::FAILED, "failed"},
    {Status::DEAD, "dead"},
})
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Job, job_id, idempotent_key, type, payload,
    priority, run_at, status, attempts, max_retries, next_retry_at)

#endif