#include<iostream>
#include<chrono>
// #include "../common/Job.h"
#include<thread>
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
std:: string R_queue:: R_queue_push(Job job){xds
    
        std::string job_id= std::to_string(job.job_id);
        auto now= std::chrono::system_clock::now();
        auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        uint64_t wake_at_ms=0;
        if (job.run_at > ms) {
            wake_at_ms = job.run_at;
        } 
        else if (job.next_retry_at > 0) {
            wake_at_ms = ms + (uint64_t)job.next_retry_at * 1000;
        }


        R_queue_update(job);
        redisReply* reply;
        std::string result_status;

        if(wake_at_ms>ms){
            reply=( redisReply*)redisCommand(c, "ZADD Waiting_jobs %llu %s", wake_at_ms, job_id.c_str());
        }
        else{
            double score= (double)job.priority * 1e12 + ms;
            reply= (redisReply*) redisCommand(c,"ZADD Ready_jobs %f %s", score, job_id.c_str()); 
        }
        

        
         if (reply->type == REDIS_REPLY_INTEGER) {
        result_status = "SUCCESS: Elements added/updated: " + std::to_string(reply->integer);
    } else if (reply->type == REDIS_REPLY_ERROR) {
        result_status = "REDIS ERROR: " + std::string(reply->str);
    } else {
        result_status = "UNKNOWN RESPONSE";
    }

    freeReplyObject(reply);
    return result_status;
}

// update status in the queue record
void R_queue::R_queue_update(Job job){
    std::string job_id= std::to_string(job.job_id);
    std::string job_json= json(job).dump();
    redisReply *push_job_json= (redisReply*) redisCommand(c,"Set Job:%s %s", job_id.c_str(),job_json.c_str());
    freeReplyObject(push_job_json);  
}

// pop from queue
std::optional<Job> R_queue::R_queue_pop(){
        Job result_job;
        redisReply *pop_job= (redisReply*)redisCommand(c, "ZMPOP 1 Ready_jobs MIN");
        if (pop_job == nullptr){
            return std::nullopt;
        }

        if (pop_job->type == REDIS_REPLY_NIL || pop_job->elements == 0) {
            freeReplyObject(pop_job);
            return std::nullopt;
        }

        // ZPOPMIN returns a flat array: [ "member", "score" ]
        std::string popped_job_id = pop_job->element[1]->element[0]->element[0]->str;
    
        std::cout << "Popped Job ID: " << popped_job_id << "\n";

        int retires= 3;
        redisReply *pop_job_json;
        while(retires--){
            pop_job_json= (redisReply*) redisCommand(c,"GET Job:%s",popped_job_id.c_str());

            if (pop_job_json != nullptr && pop_job_json->type != REDIS_REPLY_NIL && pop_job_json->type != REDIS_REPLY_ERROR)  break; 
            
            if (pop_job_json != nullptr) freeReplyObject(pop_job_json);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
        }
        if (pop_job_json == nullptr || pop_job_json->type == REDIS_REPLY_NIL || pop_job_json->type == REDIS_REPLY_ERROR) {
        
            freeReplyObject(pop_job); 
            return std::nullopt;
        }
        json j= json::parse(pop_job_json->str);
        result_job= j.get<Job>();
        result_job.error["Job_pop_status"] = "Sucsses";
        freeReplyObject(pop_job_json);
        freeReplyObject(pop_job);
        return result_job;
    
        
}

// delete queue record
void R_queue::R_queue_delete(JobId job_id){
    std::string job_id_str= std::to_string(job_id);
    redisReply *delete_job= (redisReply*)redisCommand(c,"DEL Job:%s", job_id_str.c_str());
    freeReplyObject(delete_job);
}

// fetch status from queue record
std::string R_queue::get_status(JobId job_id){
    std::string job_id_str=std::to_string(job_id);
    redisReply *get_status= (redisReply*)redisCommand(c, "GET Job:%s", job_id_str.c_str());
    if(get_status == nullptr || get_status->type == REDIS_REPLY_NIL){
        freeReplyObject(get_status);
        return "";  // empty = not in Redis, check Postgres
    }
    json j= json::parse(get_status->str);
    
    freeReplyObject(get_status);
    return j["status"];
}

// genrate id for new job
JobId R_queue::genrate_id(){
    redisReply* r= (redisReply*)redisCommand(c, "Incr job:counter");
    JobId id= r->integer;
    freeReplyObject(r);
    return id;
}

// lock job with worker 
bool R_queue::R_queue_lockJob(JobId job_id, JobId worker_id){
    std::string job_id_str= std::to_string(job_id);
    std::string worker_id_str= std::to_string(worker_id);

    redisReply* lock_job= (redisReply*)redisCommand(c,"Set lockJob:%s %s NX EX 30", job_id_str.c_str(),worker_id_str.c_str());
    bool aquired= (lock_job->type== REDIS_REPLY_STATUS);
    freeReplyObject(lock_job);
    return aquired;
}

// unlock job 
void R_queue::R_queue_unlock(JobId job_id){
    std::string job_id_str= std::to_string(job_id);

    redisReply* unlock= (redisReply*)redisCommand(c,"Del lockJob:%s", job_id_str.c_str());

    freeReplyObject(unlock);
}