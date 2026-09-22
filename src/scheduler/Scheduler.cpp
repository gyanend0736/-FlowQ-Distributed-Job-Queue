#include "Scheduler.h"

Scheduler::Scheduler():running(false),pool(std::make_unique<RedisConnectionPool>("localhost",6379,1)), q(*pool){
}
void Scheduler::start(){
    running= true;
    sched_thread= std::thread([this](){
        while(running){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait_for(lock,std::chrono::seconds(1), [this](){
                return !running.load();
            });
            if(!running) break;
            tick();
        }
        
    });
    
}


void Scheduler::stop(){
    if(!running) return;
    running= false;
    cv.notify_one();
    if(sched_thread.joinable()){
    sched_thread.join(); // ← wait for it to finish cleanly
    }
}


void Scheduler::tick(){
    auto now= std::chrono::system_clock::now();
    auto ms= std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    RedisConnGuard guard(*pool);
    redisReply* due= (redisReply*)redisCommand(guard.get(), "ZRANGEBYSCORE Waiting_jobs 0 %llu",ms);
    if(due == NULL || due->type== REDIS_REPLY_NIL || due->elements==0){
        freeReplyObject(due);
        return;
    }
    for(size_t i=0;i<due->elements;i++){
        std::string job_id= due->element[i]->str;
        redisReply* job_json= (redisReply*)redisCommand(guard.get(), "Get Job:%s", job_id.c_str());
         if(job_json == NULL || job_json->type== REDIS_REPLY_NIL){
            
            freeReplyObject(job_json);
            continue;
        }
        Job job= json::parse(job_json->str);
        freeReplyObject(job_json);
        job.next_retry_at= 0;
        q.R_queue_push(job);
        redisCommand(guard.get(), "ZREM Waiting_jobs %s", job_id.c_str());

    }
    freeReplyObject(due);
}

int main(){
    std::unique_ptr<Scheduler> s;
    try {
        s = std::make_unique<Scheduler>();
    } catch (const std::exception& e) {
        std::cerr << "FATAL: could not start flowq_Scheduler - " << e.what() << "\n";
        return 1;
    }  
    s->start();
    std::cout << "Scheduler running. Press Enter to stop.\n";
    std::cin.get();
    s->stop();
}