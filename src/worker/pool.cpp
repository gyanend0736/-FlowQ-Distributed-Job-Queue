#include<iostream>
#include "Worker.h"
#include <thread>
#include <vector>
#include<atomic>

class workerPool{
public:
    std::vector<std::thread> threads;
    std::atomic<bool> running = true;

    void start(int n){
        JobId base_worker= 10000000ULL;
        for(int i=0;i<n;i++){
            threads.emplace_back([this, base_worker, i](){
                Worker newWorker;
                newWorker.workerId= base_worker+i;
                newWorker.run(running);
            });
           
        }
    }

    void stop(){
        running= false;
        for(auto& thread:threads){
            thread.join();
        }
    }
};

int main(){
    workerPool newPool;
    newPool.start(5);

    std::cout << "Pool running with 5 workers. Press Enter to stop.\n";
    std::cin.get();  // blocks until you press Enter

    newPool.stop();
    std::cout << "All workers stopped cleanly.\n";
}