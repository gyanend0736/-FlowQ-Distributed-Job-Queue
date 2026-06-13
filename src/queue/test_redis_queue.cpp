#include <iostream>
#include<string>
#include "../common/Job.h"
#include<hiredis/hiredis.h>
#include<chrono>

class R_queue{
public:
    redisContext *c;
    R_queue(){
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
    ~R_queue(){
        if(c!=NULL){
            redisFree(c);
        }
    }
   
    std::string R_queue_push(const Job& job){
        auto job_id = std::to_string(job.job_id);
        
        auto now= std::chrono::system_clock::now();
        auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        double score= (double)job.priority * 1e12 + ms;
        std::string status_str = (job.status == Status::PENDING) ? "pending" : "other";
        // push job id in queue 
        redisReply *reply_push_Queue= (redisReply*) redisCommand(c,"ZADD Job %f %s", score, job_id.c_str());

        // push whole job to hash
        redisReply *reply_push_job= (redisReply*) redisCommand(c,"HSET Job:%llu  job_id %llu idempotent_key %s type %s payload %s priority %f run_at %llu status %s attempt %d max_retries %d next_retry_at %d",
        job.job_id,
        job.job_id, 
        job.idempotent_key.c_str(), 
        job.type.c_str(), 
        job.payload.dump().c_str(),
        (double)job.priority, 
        job.run_at, 
        status_str.c_str(), 
        job.attempts, 
        job.max_retries, 
        job.next_retry_at);

        std::string result_status;
        if (reply_push_Queue->type == REDIS_REPLY_INTEGER) {
            // ZADD returns an integer (1 if added, 0 if already existed/updated)
            result_status = "SUCCESS: Elements added/updated: " + std::to_string(reply_push_Queue->integer);
        } 
        else if (reply_push_Queue->type == REDIS_REPLY_ERROR) {
            result_status = "REDIS ERROR: " + std::string(reply_push_Queue->str);
        } 
        else {
            result_status = "UNKNOWN RESPONSE";
        }

        // Always free the pointer before exiting the function
        freeReplyObject(reply_push_Queue); 
        freeReplyObject(reply_push_job); // add this
        return result_status;
    }
    void R_queue_pop(){
        redisReply *reply= (redisReply*)redisCommand(c, "ZMPOP 1 Job MIN");
        if (reply == nullptr) return;

        if (reply->type == REDIS_REPLY_NIL || reply->elements == 0) {
            std::cout << "Queue is empty.\n";
            freeReplyObject(reply);
            return;
        }

        // ZPOPMIN returns a flat array: [ "member", "score" ]
        std::string popped_job_id = reply->element[1]->element[0]->element[0]->str;
        std::string popped_score  = reply->element[1]->element[0]->element[1]->str;

        std::cout << "Popped Job ID: " << popped_job_id << " with Score: " << popped_score << "\n";

        freeReplyObject(reply);
        
    }
    
   
};

int main(){
    R_queue redisQ;
    Job payoutJob; // Changed type to lowercase 'job' to match your header

    // 1. Core Identification
    payoutJob.job_id = 98227410293ULL;
    payoutJob.idempotent_key = "tx_unq_908123a8f"; 
    payoutJob.type = "process_payment";

    // 2. Structured JSON Payload data
    payoutJob.payload = {
        {"vendor_id", "vnd_8821"},
        {"amount", 15450.00},
        {"currency", "INR"},
        {"bank_route", "HDFC0000123"}
    };

    // 3. Execution Control & Scheduling
    payoutJob.priority = 1; 
    payoutJob.run_at = 0;   

    // 4. State Management
    payoutJob.status = Status::PENDING;
    payoutJob.attempts = 0;
    payoutJob.max_retries = 3;
    payoutJob.next_retry_at = 0;

    // 5. Precise System Time Tracking
    payoutJob.created_at = std::chrono::system_clock::now();
    payoutJob.started_at = std::chrono::system_clock::time_point();   
    payoutJob.completed_at = std::chrono::system_clock::time_point(); 

    // 6. Post-execution Metadata placeholders
    payoutJob.result = {}; 
    payoutJob.error = {};  
    
    redisQ.R_queue_push(payoutJob);
    // redisQ.pop();
    return 0;
}


