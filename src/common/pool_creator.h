#ifndef Pool_CREATOR_H
#define Pool_CREATOR_H


#include<memory>
#include "../db/postgres_connection_pool.h"
#include "../queue/redis_connection_pool.h"
#include <string>

std::unique_ptr<RedisConnectionPool> create_redis_pool_or_die(std::string binary_name, std::string host, int port, size_t pool_size);

std::unique_ptr<PostgresConnectionPool> create_postgres_pool_or_die(std::string binary_name, size_t pool_size);




#endif