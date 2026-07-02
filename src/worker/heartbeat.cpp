#include <thread>
#include <vector>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include<hiredis/hiredis.h>
#include "../common/Job.h"

class Heartbeat {
public:
    redisContext* c;
    JobId job_id;
    std::atomic<bool> running;
    std::condition_variable cv;
    std::mutex mtx;
    std::thread hb_thread;

    // constructor — just store what it needs, don't start yet
    Heartbeat(redisContext* c, JobId job_id) 
        : c(c), job_id(job_id), running(false) {}

    // destructor — make sure thread is stopped cleanly
    ~Heartbeat(){
        stop();  // safe to call even if already stopped
    }

    void start(){
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
        
    };
    void stop(){
        if(!running) return;
        running= false;
        cv.notify_one();
        if(hb_thread.joinable()){
        hb_thread.join(); // ← wait for it to finish cleanly
        }
    };
};
