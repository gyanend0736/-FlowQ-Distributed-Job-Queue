#include<iostream>
#include<chrono>
#include "../common/Job.h"
#include "redis_queue.h"

//constructor
R_queue::R_queue(){
    c= redisConnect("127.0.0.1",6379);
    if (c == NULL || c->err) {
        if (c != NULL) {
            printf("Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
        exit(1);
    }
}

//destructor
R_queue:: ~R_queue(){
    if(c!=NULL){
        redisFree(c);
    }
}

// push for queue
std:: string R_queue:: R_queue_push(Job job){
    std::string Job_payload= std::to_string(job.job_id);
        auto now= std::chrono::system_clock::now();
        auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        double score= (double)job.priority * 1e12 + ms;

        redisReply *reply= (redisReply*) redisCommand(c,"ZADD Job %f %s", score, Job_payload.c_str());
        std::string result_status;

        if (reply->type == REDIS_REPLY_INTEGER) {
            // ZADD returns an integer (1 if added, 0 if already existed/updated)
            result_status = "SUCCESS: Elements added/updated: " + std::to_string(reply->integer);
        } 
        else if (reply->type == REDIS_REPLY_ERROR) {
            result_status = "REDIS ERROR: " + std::string(reply->str);
        } 
        else {
            result_status = "UNKNOWN RESPONSE";
        }

        // Always free the pointer before exiting the function
        freeReplyObject(reply); 
        
        return result_status;
}

// pop from queue
std::string R_queue::R_queue_pop(){
        redisReply *reply= (redisReply*)redisCommand(c, "ZMPOP 1 Job MIN");
        if (reply == nullptr) return "";

        if (reply->type == REDIS_REPLY_NIL || reply->elements == 0) {
            std::cout << "Queue is empty.\n";
            freeReplyObject(reply);
            return "";
        }

        // ZPOPMIN returns a flat array: [ "member", "score" ]
        std::string popped_job_id = reply->element[1]->element[0]->element[0]->str;
        std::string popped_score  = reply->element[1]->element[0]->element[1]->str;

        std::cout << "Popped Job ID: " << popped_job_id << " with Score: " << popped_score << "\n";

        freeReplyObject(reply);
        return popped_job_id;
        
    }