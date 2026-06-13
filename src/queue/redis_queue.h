#ifndef R_QUEUE_H
#define R_QUEUE_H
#include<hiredis/hiredis.h>
#include "../common/Job.h"
#include<string>
struct R_queue{
public:

    redisContext *c;
    R_queue();
    ~R_queue();
    std::string R_queue_push(Job job);
    std::string R_queue_pop();
};




#endif