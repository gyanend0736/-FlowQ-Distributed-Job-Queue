#include<iostream>
#include "Worker.h"
#include<thread>
#include "heartbeat.cpp"

using namespace std;


//constructor
Worker::Worker(){
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
Worker:: ~Worker(){
    if(c!=NULL){
        redisFree(c);
    }
}

bool Worker::process(Job& job){
    Heartbeat hd(c, job.job_id);
    hd.start();
    std::cout << "Processing job: " << job.job_id << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(40));
    hd.stop();
    return true;  
}

void Worker::handle_faliure(Job& job, R_queue& q, DB& db){
       job.attempts+=1;
       job.next_retry_at= (job.next_retry_at==0)? 1: 2*job.next_retry_at;
       if(job.max_retries>job.attempts){
            job.status= Status::PENDING;
            q.R_queue_push(job);
       }
       else{
            job.status= Status::DEAD;
            
            q.R_queue_delete(job.job_id);
            db.insert_dead_job(job);
       }
}

void Worker::run(std::atomic<bool>& running){
    R_queue q;
    DB db;
    while(running){
        auto job = q.R_queue_pop();

        if(job.has_value()){
            Job j = job.value(); 
            bool locked= q.R_queue_lockJob(j.job_id, workerId);
            if(!locked){
                std::cout << "Job already locked, skipping\n";
                continue;
            }
            bool Sucsses= process(j);
            if(!Sucsses){
                j.status=Status::FAILED;
                handle_faliure(j, q,db);
            }
            else{
                j.status= Status::DONE;
                q.R_queue_delete(j.job_id);
                db.update_job(j);
            }
            q.R_queue_unlock(j.job_id);
        }
        else{
            cout<<"queue is empty"<< "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    } 
}

