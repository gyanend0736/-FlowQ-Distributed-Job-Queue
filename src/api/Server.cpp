#include "crow_all.h" 
#include "../queue/redis_queue.h"
#include "../db/postgres.h"
#include "../common/Job.h"
#include "nlohmann/json.hpp"
#include "../common/Priority.h"
#include "../queue/redis_connection_pool.h"
#include "../common/pool_creator.h"


using json= nlohmann::json;
int main(){
    crow::SimpleApp app;

    auto redis_pool= create_redis_pool_or_die("flowq_api", "localhost", 6379, 12);
    auto postgres_pool= create_postgres_pool_or_die("flowq_api",12);
    R_queue queu(*redis_pool);
    DB db(*postgres_pool);

// ------------------------------ Job Post request ----------------------------------------------

    CROW_ROUTE(app, "/jobs").methods("POST"_method)([&queu, &db](const crow::request& req){
        auto body= crow::json::load(req.body);
        if(!body){return crow::response(400, "invalid json");}

        Job job;
        job.job_id= queu.genrate_id();
        // job.client_id= body["client_id"].s();
        job.type= body["type"].s();
        job.attempts=0;
        job.max_retries=5;
        job.next_retry_at=0;
        job.status= Status::PENDING;
        crow::json::wvalue wval(body["payload"]);
        
        job.payload= nlohmann::json::parse(wval.dump());
        
        job.priority= (body.has("priority"))? priority_from_string(body["priority"].s()) : 3;
        
        job.run_at= (body.has("run_at"))? body["run_at"].i() : 0;
        
        
        // job.idempotent_key= (body.has("idempotent_key"))? body["idempotent_key"].s() : '';
        db.insert_job(job);
        queu.queue_push(job);
        

        crow::json::wvalue res;
        res["job_id"]= job.job_id;
        res["status"]= "pending";
        return crow::response(202, res);
    });
    
// ------------------------------Job status request ----------------------------------------------

    CROW_ROUTE(app, "/jobs/<string>")([&queu, &db](std::string job_id_str){
        JobId job_id = std::stoull(job_id_str);
        std::string status= queu.get_status(job_id);
        
        if(!status.empty()){
            crow::json::wvalue res;
            res["job_id"]= job_id;
            res["status"]= status;
            return crow::response(200, res);
        }
        // search in db
        std::string pg_job =db.get_status(job_id);
        if(pg_job.empty()){
            return crow::response(404, "job not found");
        }
        
        
        
        return crow::response(200, pg_job);
        
    });

    app.port(8080).multithreaded().run();
}