#ifndef POSTGRES_CONN_H
#define POSTGRES_CONN_H

#include<mutex>
#include<vector>
#include<condition_variable>
#include <libpq-fe.h>


class PostgresConnectionPool{
public:
    PostgresConnectionPool(size_t pool_size);
    ~PostgresConnectionPool();
    PGconn* acquier();
    void release(PGconn* conn);
private:
    std::vector<PGconn*> available;
    std::mutex mtx;
    std::condition_variable cv;
};

class PostgresConnGuard{
    public:
    PostgresConnGuard(PostgresConnectionPool& pool);
    ~PostgresConnGuard();
    PGconn* get();
    private:
    PostgresConnectionPool& pool;
    PGconn* conn;

};

#endif
