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
    
    try{
        // fetching all the jobs from waiting_jobs whose run_at is less than current time
        std::vector<std::string> due_jobs= q.get_due_waitingJobs();
   
        if(!due_jobs.empty()){
            for(size_t i=0;i<due_jobs.size();i++){
                
                // fetching the jobs detail from redis
                std::string job_id= due_jobs[i];
            
                auto job_opt= q.queue_get_JobRecords(job_id);
                if(job_opt.has_value()){
                    Job job= job_opt.value();
                    // push in the ready queue
                    job.next_retry_at= 0;
                    q.queue_push(job);

                    // delete from waiting jobs 
                    q.delete_from_waitingJobs(job_id);
                }
                else{
                    std::cerr << "Job record not found for Job ID: " << job_id << "\n";
                }
            
            }
        }
        else{
            std::cerr << "No due waiting jobs found\n";
        }
    }
    catch(const std::exception& e){
        std::cerr << "Error in Scheduler tick: " << e.what() << "\n";
    }
      
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