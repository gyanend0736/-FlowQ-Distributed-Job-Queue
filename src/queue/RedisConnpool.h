#ifndef CONNECTION_POOL
#define CONNECTION_POOL

#include<hiredis/hiredis.h>
#include<condition_variable>
#include <mutex>
#include <vector>
#include<string>

class RedisConnectionPool{
    public:
        RedisConnectionPool(const std::string& host, int port, size_t pool_size);
        ~RedisConnectionPool();
        redisContext* acquire();
        void release(redisContext* c);

    private:
        redisContext* conn;
        std::vector<redisContext*> available;
        std::mutex mtx;
        std::condition_variable cv;
};

class RedisConnGaurd{
    public:
        RedisConnGaurd();
        ~RedisConnGaurd();
        redisContext* get();
    private:
        RedisConnectionPool& pool;
        redisContext* conn;
}



#endif