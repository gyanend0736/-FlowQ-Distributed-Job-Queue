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
    std::optional<Job> R_queue_pop();
    void R_queue_delete(JobId job_id);
    void R_queue_dead(Job job);
};




#endif