#include "crow_all.h" 
#include "../queue/redis_queue.h"
#include "../db/postgres.h"
#include "../common/Job.h"
#include "nlohmann/json.hpp"
using json= nlohmann::json;
int main(){
    crow::SimpleApp app;
    R_queue queu;
    DB db;

    CROW_ROUTE(app, "/jobs").methods("POST"_method)([&queu, &db](const crow::request& req){
        auto body= crow::json::load(req.body);
        if(!body){return crow::response(400, "invalid json");}

        Job job;
        job.job_id= queu.genrate_id();
        job.client_id= body["client_id"].s();
        job.type= body["type"].s();
        job.attempts=0;
        job.max_retries=5;
        job.status= Status::PENDING;
        job.payload= nlohmann::json::parse(body["payload"].dump());
        
        queu.R_queue_push(job);
        db.insert_job(job);

        crow::json::wvalue res;
        res["job_id"]= job.job_id;
        res["status"]= "pending";
        return crow::response(202, res);
    });
    
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