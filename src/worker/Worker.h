#ifndef Worker_h
#define Worker_h

#include<iostream>
#include<string>    
#include<hiredis/hiredis.h>
#include "../queue/redis_queue.h"
#include "../db/postgres.h"
#include <atomic>
#include "../queue/redis_connection_pool.h"
#include "../db/postgres_connection_pool.h"
#include "../common/pool_creator.h"

struct Worker
{ 

    RedisConnectionPool& redis_pool;
    PostgresConnectionPool& postgres_pool;
    JobId workerId;
    Status status;
    JobId current_jobId;
    Worker(RedisConnectionPool& redis_pool, PostgresConnectionPool& postgres_pool);
    ~Worker();
    bool process(Job& job);
    void handle_faliure(Job& job, R_queue& q, DB& db);
    void run(std:: atomic<bool>& running);

};
#endif
