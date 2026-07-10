#include "postgres.h"

DB::DB(){
    conn= PQconnectdb("host=127.0.0.1 port=5431 user=flowq password=gyanend0736 dbname=flowq");
    if(PQstatus(conn) != CONNECTION_OK){
        printf("error in connecting db\n");
    }
}
DB::~DB(){
    PQfinish(conn);
}

void DB::insert_job(Job job){
    std::string s_job_id= std::to_string(job.job_id);
    std::string s_payload= job.payload.dump();
    std::string s_priority= std::to_string(job.priority);
    std::string s_status= "pending";
    std::string s_attempts= std::to_string(job.attempts);
    std::string s_max_retries= std::to_string(job.max_retries);
    const char* values[]= {
        s_job_id.c_str(),
        job.idempotent_key.c_str(),
        job.type.c_str(),
        s_payload.c_str(),
        s_priority.c_str(),
        s_status.c_str(),
        s_attempts.c_str(),
        s_max_retries.c_str()
    };
    PGresult* res=PQexecParams(conn,
        "INSERT INTO jobs (job_id,idempotent_key, type, payload,priority, status, attempts, max_retries) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
        8,
        NULL,
        values,
        NULL,
        NULL,
        0

    );
    if(PQresultStatus(res) != PGRES_COMMAND_OK){
        std::cerr << PQerrorMessage(conn) << "\n";
    }
    PQclear(res);
}

void DB::insert_dead_job(Job job){
     std::string s_job_id= std::to_string(job.job_id);
    std::string s_payload= job.payload.dump();
    std::string s_priority= std::to_string(job.priority);
    std::string s_status= "pending";
    std::string s_attempts= std::to_string(job.attempts);
    std::string s_max_retries= std::to_string(job.max_retries);
    const char* values[]= {
        s_job_id.c_str(),
        job.idempotent_key.c_str(),
        job.type.c_str(),
        s_payload.c_str(),
        s_priority.c_str(),
        
        s_attempts.c_str(),
        
    };
    PGresult* res=PQexecParams(conn,
        "INSERT INTO dead_letter_queue (job_id,idempotent_key, type, payload,priority, attempts) "
        "VALUES ($1, $2, $3, $4, $5, $6)",
        6,
        NULL,
        values,
        NULL,
        NULL,
        0

    );
    if(PQresultStatus(res) != PGRES_COMMAND_OK){
        std::cerr << PQerrorMessage(conn) << "\n";
    }
    PQclear(res);
}

void DB::update_job(Job job){
    std::string s_status= "pending";
    std::string s_attempts= std::to_string(job.attempts);
    std::string s_job_id= std::to_string(job.job_id);
    std::string s_result = job.result.dump();
    const char* values[]= {
        s_status.c_str(),
        s_attempts.c_str(),
        s_result.c_str(),
        s_job_id.c_str()
    };
    PGresult *res= PQexecParams(conn,
        "UPDATE jobs SET status=$1, attempts=$2, result=$3, completed_at=NOW() where job_id=$4",
        4,
        NULL,
        values,
        NULL,
        NULL,
        0
    );
    PQclear(res); // add this after the error checks
}


std::string DB::get_status(JobId job_id){
    std::string job_id_str= std::to_string(job_id);
    const char* values[]= {
       
        job_id_str.c_str()
    };
    PGresult *res= PQexecParam(conn,
        "Select status from jobs where job_id==$1",
        1,
        NULL;
        [job_id_str.c_str()],
        NULL;
        NULL;
        0
    );

    return res->status->str;
}

