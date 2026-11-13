#ifndef R_QUEUE_H
#define R_QUEUE_H
#include<hiredis/hiredis.h>
#include "../common/Job.h"
#include<string>
#include <optional>
#include "redis_connection_pool.h"

struct R_queue{
    
public:
    RedisConnectionPool& pool;
    R_queue(RedisConnectionPool& pool): pool(pool){};
    ~R_queue(){};

    // functionality for ready queue
    std::string queue_push(Job job);
    std::optional<Job> queue_pop_ready();

    // functionalities for job records
    void queue_records_update(Job job);
    void queue_delete_records(JobId job_id);
    std::optional<Job> queue_get_JobRecords(std::string job_id);
    std::string get_status(JobId job_id);
    
    // functionalities for waiting jobs
    std::vector<std::string> get_due_waitingJobs();
    void delete_from_waitingJobs(std::string job_id);

    // other functionalities
    bool queue_lockJob(JobId job_id, JobId worker_id);
    void queue_unlock(JobId job_id);
    
    JobId genrate_id();
};


#endif