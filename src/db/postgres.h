#ifndef POSTGRES_H
#define POSTGRES_H
#include<iostream>
#include<string>
#include <libpq-fe.h>
#include "../common/Job.h"

struct DB{
public:
    PGconn* conn;
    DB();
    ~DB();
    void insert_job(Job job);
    void update_job(Job job);
    void insert_dead_job(Job job);
    std::string get_status(JobId job_id);
};





#endif