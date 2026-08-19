#ifndef R_QUEUE_H
#define R_QUEUE_H
#include<hiredis/hiredis.h>
#include "../common/Job.h"
#include<string>
#include <optional>

struct R_queue{
public:

    redisContext *c;
    R_queue();
    ~R_queue();
    std::string R_queue_push(Job job);
    void R_queue_update(Job job);
    std::optional<Job> R_queue_pop();
    void R_queue_delete(JobId job_id);
    bool R_queue_lockJob(JobId job_id, JobId worker_id);
    void R_queue_unlock(JobId job_id);
    std::string get_status(JobId job_id);
    JobId genrate_id();
};


#endif