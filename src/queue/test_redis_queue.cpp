#include <iostream>
#include<string>
#include "../common/Job.h"
#include<hiredis/hiredis.h>
#include<chrono>
// #include "../db/postgres.h";


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
        // attempt1 using Hset;
        // auto job_id = std::to_string(job.job_id);
        
        // auto now= std::chrono::system_clock::now();
        // auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        // double score= (double)job.priority * 1e12 + ms;
        // std::string status_str = (job.status == Status::PENDING) ? "pending" : "other";
        // // push job id in queue 
        // redisReply *reply_push_Queue= (redisReply*) redisCommand(c,"ZADD Job %f %s", score, job_id.c_str());

        // // push whole job to hash
        // redisReply *reply_push_job= (redisReply*) redisCommand(c,"HSET Job:%s  job_id %llu idempotent_key %s type %s payload %s priority %f run_at %llu status %s attempt %d max_retries %d next_retry_at %d",
        // job_id.c_str(),
        // job.job_id, 
        // job.idempotent_key.c_str(), 
        // job.type.c_str(), 
        // job.payload.dump().c_str(),
        // (double)job.priority, 
        // job.run_at, 
        // status_str.c_str(), 
        // job.attempts, 
        // job.max_retries, 
        // job.next_retry_at);

        // std::string result_status;
        // if (reply_push_Queue->type == REDIS_REPLY_INTEGER) {
        //     // ZADD returns an integer (1 if added, 0 if already existed/updated)
        //     result_status = "SUCCESS: Elements added/updated: " + std::to_string(reply_push_Queue->integer);
        // } 
        // else if (reply_push_Queue->type == REDIS_REPLY_ERROR) {
        //     result_status = "REDIS ERROR: " + std::string(reply_push_Queue->str);
        // } 
        // else {
        //     result_status = "UNKNOWN RESPONSE";
        // }

        // // Always free the pointer before exiting the function
        // freeReplyObject(reply_push_Queue); 
        // freeReplyObject(reply_push_job); // add this
        // return result_status;


        // attempt 2 using string json method
        std::string job_id= std::to_string(job.job_id);
        std::string job_json= json(job).dump();
        auto now= std::chrono::system_clock::now();
        auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        double score= (double)job.priority * 1e12 + ms;

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


    void R_queue_pop(){
        // attempt 1 pop
        // redisReply *pop_job= (redisReply*)redisCommand(c, "ZMPOP 1 Job MIN");
        
        // if (pop_job == nullptr) return;

        // if (pop_job->type == REDIS_REPLY_NIL || pop_job->elements == 0) {
        //     std::cout << "Queue is empty.\n";
        //     freeReplyObject(pop_job);
        //     return;
        // }

        // // ZPOPMIN returns a flat array: [ "member", "score" ]
        // std::string popped_job_id = pop_job->element[1]->element[0]->element[0]->str;
        // std::string popped_score  = pop_job->element[1]->element[0]->element[1]->str;

        // redisReply *job_data= (redisReply*)redisCommand(c, "HGETALL Job:%s", popped_job_id.c_str());

        // std::cout << "Popped Job ID: " << popped_job_id << " with Score: " << popped_score << "\n";
        // int count = job_data->elements;
        // for(int i=0;i<count;i+=2){
        //     std::cout<<job_data->element[i]->str<<" : "<<job_data->element[i+1]->str<<"\n";
        // }
        // freeReplyObject(job_data);
        // freeReplyObject(pop_job);
        



        // attempt 2 pop
         redisReply *pop_job= (redisReply*)redisCommand(c, "ZMPOP 1 Job MIN");
        if (pop_job == nullptr) return;

        if (pop_job->type == REDIS_REPLY_NIL || pop_job->elements == 0) {
            std::cout << "Queue is empty.\n";
            freeReplyObject(pop_job);
            return;
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
            if (pop_job_json->type == REDIS_REPLY_STRING) {
                // The string is stored in the 'str' field
                printf("Job JSON: %s\n", pop_job_json->str);
            } 
            else if (pop_job_json->type == REDIS_REPLY_NIL) {
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
        freeReplyObject(pop_job);
        freeReplyObject(pop_job_json);
        
    }
    
   
};

int main(){
    R_queue redisQ;
    Job payoutJob; // Changed type to lowercase 'job' to match your header

  
    payoutJob.job_id = 982231410293ULL;
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
    for(int i=0;i<4;i++){
        payoutJob.job_id++;
        redisQ.R_queue_push(payoutJob);
    }
    // redisQ.R_queue_pop();
    return 0;
}


