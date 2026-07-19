#include "heartbeat.h"


void Heartbeat::start(){
    running= true;
    hb_thread= std::thread([this](){
        while(running){
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait_for(lock,std::chrono::seconds(10), [this](){
                return !running.load();
            });
            if(!running) break;
                std::string job_id_str= std::to_string(job_id);
            redisReply *Heartbeat= (redisReply*) redisCommand(c, "Expire lockJob:%s 30", job_id_str.c_str());
            freeReplyObject(Heartbeat);
        }
        
    });
    
}
void Heartbeat::stop(){
    if(!running) return;
    running= false;
    cv.notify_one();
    if(hb_thread.joinable()){
    hb_thread.join(); // ← wait for it to finish cleanly
    }
}