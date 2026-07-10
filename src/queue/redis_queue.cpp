#include<iostream>
#include<chrono>
// #include "../common/Job.h"
#include "redis_queue.h"
// #include "../db/postgres.h";

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
        std::string job_id= std::to_string(job.job_id);
        std::string job_json= json(job).dump();

        auto now= std::chrono::system_clock::now();
        auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        auto delay_ms= job.next_retry_at*1000;
        double score= (double)job.priority * 1e12 + ms+ delay_ms;

        redisReply *push_job= (redisReply*) redisCommand(c,"ZADD Job %f %s", score, job_id.c_str()); 
        redisReply *push_job_json= (redisReply*) redisCommand(c,"Set Job:%s %s", job_id.c_str(),job_json.c_str());

        // DB::insert_job(job);

        std::string result_status;
        if (push_job->type == REDIS_REPLY_INTEGER) {
            // ZADD returns an integer (1 if added, 0 if already existed/updated)
            result_status = "SUCCESS: Elements added/updated: " + std::to_string(push_job->integer);
        } 
        else if (push_job->type == REDIS_REPLY_ERROR) {
            result_status = "REDIS ERROR: " + std::string(push_job->str);
        } 
        else {
            result_status = "UNKNOWN RESPONSE";
        }

        // Always free the pointer before exiting the function
        freeReplyObject(push_job); 
        
        return result_status;
}

// pop from queue
std::optional<Job> R_queue::R_queue_pop(){
        Job result_job;
        redisReply *pop_job= (redisReply*)redisCommand(c, "ZMPOP 1 Job MIN");
        if (pop_job == nullptr){
           
            return std::nullopt;
        }

        if (pop_job->type == REDIS_REPLY_NIL || pop_job->elements == 0) {
            
            freeReplyObject(pop_job);
            return std::nullopt;

        }

        // ZPOPMIN returns a flat array: [ "member", "score" ]
        std::string popped_job_id = pop_job->element[1]->element[0]->element[0]->str;
        std::string popped_score  = pop_job->element[1]->element[0]->element[1]->str;

        std::cout << "Popped Job ID: " << popped_job_id << " with Score: " << popped_score << "\n";


        redisReply *pop_job_json= (redisReply*) redisCommand(c,"GET Job:%s",popped_job_id.c_str());
        if (pop_job_json == NULL) {
            printf("Fatal Error: Failed to execute Redis command.\n");
        }
        else {
            // 3. Check the type of the reply
            
            if (pop_job_json->type == REDIS_REPLY_NIL) {
                printf("Key does not exist in Redis.\n");
            } 
            else if (pop_job_json->type == REDIS_REPLY_ERROR) {
                // Redis returned an error message
                printf("Redis Error: %s\n", pop_job_json->str);
            } 
            else {
                printf("Unexpected Redis reply type: %d\n", pop_job_json->type);
            }
        }
        json j= json::parse(pop_job_json->str);
        result_job= j.get<Job>();
        result_job.error["Job_pop_status"] = "Sucsses";
        freeReplyObject(pop_job_json);
        freeReplyObject(pop_job);
        return result_job;
        
}


void R_queue::R_queue_delete(JobId job_id){
    std::string job_id_str= std::to_string(job_id);
    redisReply *delete_job= (redisReply*)redisCommand(c,"DEL Job:%s", job_id_str.c_str());
    freeReplyObject(delete_job);
}

JobId R_queue::genrate_id(){
    redisReply* r= (redisReply*)redisCommand(c, "Incr job:counter");
    JobId id= r->integer;
    freeReplyObject(r);
    return id;
}

bool R_queue::R_queue_lockJob(JobId job_id, JobId worker_id){
    std::string job_id_str= std::to_string(job_id);
    std::string worker_id_str= std::to_string(worker_id);

    redisReply* lock_job= (redisReply*)redisCommand(c,"Set lockJob:%s %s NX EX 30", job_id_str.c_str(),worker_id_str.c_str());
    bool aquired= (lock_job->type== REDIS_REPLY_STATUS);
    freeReplyObject(lock_job);
    return aquired;
}

void R_queue::R_queue_unlock(JobId job_id){
    std::string job_id_str= std::to_string(job_id);

    redisReply* unlock= (redisReply*)redisCommand(c,"Del lockJob:%s", job_id_str.c_str());

    freeReplyObject(unlock);
}