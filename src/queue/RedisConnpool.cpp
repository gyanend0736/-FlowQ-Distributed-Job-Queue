#include "RedisConnpool.h"
#include <stdexcept>

RedisConnectionPool::RedisConnectionPool(const std::string& host, int port, size_t pool_size){
    size_t requested = pool_size;
    size_t attempts = 0;
    size_t max_attempts = 20;

    while(pool_size > 0 && attempts < max_attempts){
        attempts++;
        redisContext* conn = redisConnect(host.c_str(), port);
        if (conn == NULL || conn->err) {
            if (conn != NULL) {
                printf("Error: %s\n", conn->errstr);
                redisFree(conn);
            } else {
                printf("Can't allocate redis context\n");
            }
            continue;
        }
        available.push_back(conn);
        pool_size--;
    }

    printf("RedisConnectionPool: got %zu / %zu connections\n", available.size(), requested);

    if (available.empty()) {
        // shop opened with zero books — this should be fatal
        throw std::runtime_error("RedisConnectionPool: failed to establish any connections");
    }
}

RedisConnectionPool::~RedisConnectionPool(){
    while(!available.empty()){
        redisContext* conn = available.back();
        available.pop_back();
        redisFree(conn);
    }
}

redisContext* RedisConnectionPool::acquire(){
    
    std::unique_lock<std::mutex> lock(mtx);
    bool got_one= cv.wait_for(lock, std::chrono::seconds(2), [&](){
        return !available.empty();
    });
    if(!got_one){
        return nullptr;
    }
    redisContext* conn= available.back();
    available.pop_back();
    return conn;
}

void RedisConnectionPool::release(redisContext* conn){
    {
        std::unique_lock<std::mutex> lock(mtx);
        available.push_back(conn);
    }
    cv.notify_one();
}


//Raii wrapper 

RedisConnGuard::RedisConnGuard(RedisConnectionPool& pool) : pool(pool) {
    conn = pool.acquire();
     if (conn == nullptr) {
        throw std::runtime_error("RedisConnGuard: failed to acquire connection (pool exhausted/timeout)");
    }
}

RedisConnGuard::~RedisConnGuard(){
    pool.release(conn);
}

redisContext* RedisConnGuard::get(){
    return conn;
}