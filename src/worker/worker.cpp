#include<iostream>
#include "Worker.h"
#include<thread>


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
std::bool Worker::process(Job job){
    std::cout << "Processing job: " << job.job_id << "\n";
    std::cout << "Type: " << job.type << "\n";
    std::cout << "Payload: " << job.payload.dump() << "\n"; 
    std::cout<< "error: "<< job.error.dump()<<"\n";   
}

void Worker::handle_faliure(Job& job, R_queue& q){
       job.attempts+=1;
       job.next_retry_at= (job.next_retry_at==0)? 1: 2*job.next_retry_at;
       if(job.max_retries>job.attempts){
            job.status= Status::PENDING;
            q.R_queue_push(job);
       }
       else{
            job.status= Status::DEAD;
            q.R_queue_dead(job.job_id);
            q.R_queue_delete(job.job_id);

       }
}

void Worker::run(R_queue& q){
    while(true){
        auto job = q.R_queue_pop();
        if(job.has_value()){
            bool Sucsses= process(job.value());
            if(!Sucsses){
                job.status=Status::FAILED;
                handle_faliure(job.value(), q);
            }
            
        }
        else{
            cout<<"queue is empty"<< "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

    } 
}

int main(){
    Worker a;
    R_queue q;
    a.workerId= 248783434ULL;
    a.status= Status::PENDING;
    a.run(q);

}