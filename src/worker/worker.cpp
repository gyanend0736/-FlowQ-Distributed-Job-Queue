#include<iostream>
#include "Worker.h"



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
void Worker::process(std::string job_id){
    std::cout<< job_id;
    
}

void Worker::run(R_queue q){
    std::string job_id= q.R_queue_pop();
    process(job_id);
    
    
}

int main(){
    Worker a;
    R_queue q;
    a.workerId= 248783434ULL;
    a.status= Status::PENDING;
    a.run(q);

}