#include "RedisConnpool.h"


RedisConnectionPool::RedisConnectionPool(const std::string& host, int port, size_t pool_size){
    while(pool_size>0){
        conn = redisConnect("127.0.0.1", port);
          if (conn== NULL || conn->err) {
        if (conn!= NULL) {
            printf("Error: %s\n", conn->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
        available.push_back(conn);
    }
    }
}