#include "postgres_connection_pool.h"
#include <cstdlib>
#include <iostream>

std::string create_connection_string(){
    std::string host = std::getenv("PG_host") ? std::getenv("PG_host") : "localhost";
    std::string password = std::getenv("PG_password") ? std::getenv("PG_password") : "";
    std::string user = std::getenv("PG_user") ? std::getenv("PG_user") : "";
    std::string dbname = std::getenv("PG_dbname") ? std::getenv("PG_dbname") : "";
    std::string port = std::getenv("PG_port") ? std::getenv("PG_port") : "5432";
    return "host=" + host + " port=" + port + " user=" + user +
                                " password=" + password + " dbname=" + dbname;
}

PostgresConnectionPool::PostgresConnectionPool(size_t pool_size){
    size_t requested= pool_size;
    int attempts= 0;
    int maxattempts=20;
    std::string conn_details= create_connection_string();
    while(pool_size>0 && attempts<maxattempts){
        PGconn* conn= PQconnectdb(conn_details.c_str());
        if(PQstatus(conn) != CONNECTION_OK){
            printf("error in connecting db\n");
            attempts++;
            PQfinish(conn);
            continue;
        }
        printf("Connection found! Connection number: %zu\n", requested-pool_size);
        available.push_back(conn);
        pool_size--;
    }
    printf("PostgresConnectionPool: got %zu / %zu connections\n", available.size(), requested);

    if (available.empty()) {
        // shop opened with zero books — this should be fatal
        throw std::runtime_error("PostgresConnectionPool: failed to establish any connections");
    }
}

PostgresConnectionPool::~PostgresConnectionPool(){
    while(!available.empty()){
        PGconn* conn= available.back();
        available.pop_back();
        PQfinish(conn);
    }
}

PGconn* PostgresConnectionPool::acquier(){
    std::unique_lock<std::mutex> lock(mtx);
    bool got_one= cv.wait_for(lock, std::chrono::seconds(2), [&](){
        return !available.empty();
    });
    if(!got_one){
        return nullptr;
    }
    PGconn* conn= available.back();
    available.pop_back();
    return conn;
}

void PostgresConnectionPool::release(PGconn* conn){
    {
        std::unique_lock<std::mutex> lock(mtx);
        available.push_back(conn);
    }
    cv.notify_one();
}

// --------------------------RAII wraper---------------------------------

PostgresConnGuard::PostgresConnGuard(PostgresConnectionPool& pool) : pool(pool) {
    conn= pool.acquier();
    if(conn==nullptr){
        throw std::runtime_error("PostgresConnection: failed to acquire connection (pool exhausted/timeout)");
    }
}

PostgresConnGuard::~PostgresConnGuard(){
    pool.release(conn);
}

PGconn* PostgresConnGuard::get(){
    return conn;
}

