#include<iostream>
#include<chrono>
#include<thread>
#include "redis_queue.h"
#include "redis_connection_pool.h"

//1.1 push for queue
std:: string R_queue:: queue_push(Job job){
        
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

        queue_records_update(job);
        
        redisReply* reply;
        std::string result_status;
        RedisConnGuard guard(pool);

        if(wake_at_ms>ms){
            reply=( redisReply*)redisCommand(guard.get(), "ZADD Waiting_jobs %llu %s", wake_at_ms, job_id.c_str());
        }
        else{
            double score= (double)job.priority * 1e12 + ms;
            reply= (redisReply*) redisCommand(guard.get(),"ZADD Ready_jobs %f %s", score, job_id.c_str()); 
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

//1.2 pop from ready queue
std::optional<Job> R_queue::queue_pop_ready(){
        RedisConnGuard guard(pool);
        Job result_job;
        redisReply *pop_job= (redisReply*)redisCommand(guard.get(), "ZMPOP 1 Ready_jobs MIN");
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
            pop_job_json= (redisReply*) redisCommand(guard.get(),"GET Job:%s",popped_job_id.c_str());

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


//2.1 update status in the queue record
void R_queue::queue_records_update(Job job){
    RedisConnGuard guard(pool);
    std::string job_id= std::to_string(job.job_id);
    std::string job_json= json(job).dump();
    redisReply *reply= (redisReply*) redisCommand(guard.get(),"Set Job:%s %s", job_id.c_str(),job_json.c_str());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Error updating job record for Job ID: " << job_id << "\n";
    }
    freeReplyObject(reply);  
}

//2.2 fetch job record from queue
std::optional<Job> R_queue::queue_get_JobRecords(std::string job_id){
    RedisConnGuard guard(pool);
    
    redisReply* reply= (redisReply*) redisCommand(guard.get(), "GET Job:%s", job_id.c_str());
    if(reply == nullptr || reply->type== REDIS_REPLY_ERROR){
        freeReplyObject(reply);
        throw std::runtime_error("Error fetching job record for Job ID: " + job_id);        
    }
    if(reply->type== REDIS_REPLY_NIL){
        freeReplyObject(reply);

        return std::nullopt;
    }
    json j= json::parse(reply->str);
    freeReplyObject(reply);
    return j.get<Job>();
}

//2.3 delete queue record
void R_queue::queue_delete_records(JobId job_id){
    RedisConnGuard guard(pool);
    std::string job_id_str= std::to_string(job_id);
    redisReply *reply= (redisReply*)redisCommand(guard.get(),"DEL Job:%s", job_id_str.c_str());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Error deleting job record for Job ID: " << job_id_str << "\n";
    }
    freeReplyObject(reply);
}

//2.4 fetch status from queue record
std::string R_queue::get_status(JobId job_id){
    RedisConnGuard guard(pool);
    std::string job_id_str=std::to_string(job_id);
    redisReply *get_status= (redisReply*)redisCommand(guard.get(), "GET Job:%s", job_id_str.c_str());
    if(get_status == nullptr || get_status->type == REDIS_REPLY_NIL){
        freeReplyObject(get_status);
        return "";  // empty = not in Redis, check Postgres
    }
    json j= json::parse(get_status->str);
    
    freeReplyObject(get_status);
    return j["status"];
}



//3.1 get all the jobs from waiting_jobs whose run_at is less than current time
std::vector<std::string> R_queue::get_due_waitingJobs(){  
    RedisConnGuard guard(pool);
    auto now= std::chrono::system_clock::now();
    auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    redisReply* reply= (redisReply*)redisCommand(guard.get(), "ZRANGEBYSCORE Waiting_jobs 0 %lld",ms);
    if(reply == NULL || reply->type== REDIS_REPLY_ERROR){
        freeReplyObject(reply);
        throw std::runtime_error("Error fetching due waiting jobs from Redis");
    }
    if(reply->elements==0){
        freeReplyObject(reply);
        return {};
    }
    std::vector<std::string> due_jobs;
    for(size_t i=0;i<reply->elements;i++){
        due_jobs.push_back(reply->element[i]->str);

    }
    freeReplyObject(reply);
    return due_jobs;
}

//3.2 delete from waiting_jobs
void R_queue::delete_from_waitingJobs(std::string job_id){
    RedisConnGuard guard(pool);
    redisReply* reply= (redisReply*)redisCommand(guard.get(),"ZREM Waiting_jobs %s", job_id.c_str());
    if(reply==NULL || reply->type== REDIS_REPLY_NIL){
        std::cerr<<"Error deleting job from waiting_jobs: "<<job_id<<"\n";
    }
    freeReplyObject(reply);
}


//4.1 genrate id for new job
JobId R_queue::genrate_id(){
    RedisConnGuard guard(pool);
    redisReply* reply= (redisReply*)redisCommand(guard.get(), "Incr job:counter");
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Error generating new job ID\n";
    }
    JobId id= reply->integer;
    freeReplyObject(reply);
    return id;
}

//4.2 lock job with worker 
bool R_queue::queue_lockJob(JobId job_id, JobId worker_id){
    RedisConnGuard guard(pool);
    std::string job_id_str= std::to_string(job_id);
    std::string worker_id_str= std::to_string(worker_id);

    redisReply* reply= (redisReply*)redisCommand(guard.get(),"Set lockJob:%s %s NX EX 30", job_id_str.c_str(),worker_id_str.c_str());
    bool aquired= (reply->type== REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return aquired;
}

//4.3 unlock job 
void R_queue::queue_unlock(JobId job_id){6
    RedisConnGuard guard(pool);
    std::string job_id_str= std::to_string(job_id);

    redisReply* reply= (redisReply*)redisCommand(guard.get(),"Del lockJob:%s", job_id_str.c_str());
    if(reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        std::cerr << "Error unlocking job for Job ID: " << job_id_str << "\n";
    }
    freeReplyObject(reply);
}