#include<iostream>
#include "Worker.h"
#include <thread>
#include <vector>
#include <atomic>
#include "../queue/redis_connection_pool.h"
#include "../db/postgres_connection_pool.h"

class workerPool{
public:
    std::vector<std::thread> threads;
    std::atomic<bool> running = true;
    std::unique_ptr<RedisConnectionPool> redis_pool;
    std::unique_ptr<PostgresConnectionPool> postgres_pool;
    void start(int n){
        JobId base_worker= 10000000ULL;
        for(int i=0;i<n;i++){
            threads.emplace_back([this, base_worker, i](){
                Worker newWorker(*redis_pool,*postgres_pool);
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
    int poolSize=5;
    newPool.postgres_pool= create_postgres_pool_or_die("flowq_worker", 5);
    newPool.redis_pool= create_redis_pool_or_die("flowq_worker", "localhost", 6379, 5);
    newPool.start(poolSize);

    std::cout << "Pool running with 5 workers. Press Enter to stop.\n";
    std::cin.get();  // blocks until you press Enter

    newPool.stop();
    std::cout << "All workers stopped cleanly.\n";
}