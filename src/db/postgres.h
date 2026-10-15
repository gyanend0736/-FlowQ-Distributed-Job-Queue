#ifndef POSTGRES_H
#define POSTGRES_H
#include<iostream>
#include<string>
#include <libpq-fe.h>
#include "../common/Job.h"
#include "postgres_connection_pool.h"

struct DB{
public:
    PostgresConnectionPool& pool;
    DB(PostgresConnectionPool& pool): pool(pool){};
    ~DB();
    void insert_job(Job job);
    void update_job(Job job);
    void insert_dead_job(Job job);
    std::string get_status(JobId job_id);
};





#endif