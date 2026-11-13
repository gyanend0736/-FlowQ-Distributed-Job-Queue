#include<iostream>
#include "Worker.h"
#include<thread>
#include "heartbeat.h"
#include <math.h>
using namespace std;


//constructor
Worker::Worker(RedisConnectionPool& redis_pool, PostgresConnectionPool& postgres_pool):redis_pool(redis_pool), postgres_pool(postgres_pool){
}

//destructor
Worker:: ~Worker(){
}

bool Worker::process(Job& job){
    Heartbeat hd(redis_pool, job.job_id);
    hd.start();
    std::cout << "Processing job: " << job.job_id << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    hd.stop();
    return true;  
}

void Worker::handle_faliure(Job& job, R_queue& q, DB& db){
       job.attempts+=1;
       job.next_retry_at= 1<< job.attempts;
       if(job.max_retries>job.attempts){
            job.status= Status::PENDING;
            q.queue_push(job);
       }
       else{
            job.status= Status::DEAD;
            db.insert_dead_job(job);
            q.queue_delete_records(job.job_id);
            
       }
}

void Worker::run(std::atomic<bool>& running){
    R_queue q(redis_pool);
    DB db(postgres_pool);
     bool waiting_message_shown = false; 
     
    while(running){
        auto job = q.queue_pop_ready();

        if(job.has_value()){
            Job j = job.value(); 
            bool locked= q.queue_lockJob(j.job_id, workerId);
            if(!locked){
                std::cout << "Job already locked, skipping\n";
                continue;
            }
            if(j.attempts==0){
                auto now= std::chrono::system_clock::now();
                auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                j.started_at= ms;
            }
            j.status= Status::PROCESSING;
            q.queue_records_update(j);
            bool Sucsses= process(j);
            if(!Sucsses){
                j.status=Status::FAILED;
                handle_faliure(j, q,db);
            }
            else{
                j.status= Status::DONE;
                q.queue_delete_records(j.job_id);
                db.update_job(j);
            }
            q.queue_unlock(j.job_id);
        }
        else{
            if(!waiting_message_shown){
                std::cout << "Waiting for job";
                std::cout.flush();
                waiting_message_shown = true;
            } else {
                std::cout << ".";
                std::cout.flush();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
       

    } 
}

