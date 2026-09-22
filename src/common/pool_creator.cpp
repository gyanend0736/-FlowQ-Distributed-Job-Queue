#include "pool_creator.h"
#include <iostream>

std::unique_ptr<RedisConnectionPool> create_redis_pool_or_die(string:: binary_name, string::host, int port, size_t pool_size){
    try{
        return std::make_unique<RedisConnectionPool>(host, port, pool_size);
    }catch(const std::exception& e){
        std::cerr << binary_name << ": failed to create Redis connection pool: " << e.what() << "\n";
        exit(1);
    }
}

std::unique_ptr<PostgresConnectionPool> create_postgres_pool_or_die(string:: binary_name, size_t pool_size){
    try{
        return std::make_unique<PostgresConnectionPool>(pool_size);
    }catch(const std::exception& e){
        std::cerr << binary_name << ": failed to create Postgres connection pool: " << e.what() << "\n";
        exit(1);
    }
}