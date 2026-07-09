#include "crow_all.h" 
#include "queue/redis_queue.h"
#include "db/postgres.h"
#include "worker/Worker.h"
#include "common/Job.h"
int main(){
    crow::SimpleApp app;
    R_queue queu;
    DB db;

    CROW_ROUTE(app, "/jobs").methods("PUSH"_method)([&queu, &db](const crow::request& req){
        auto body= crow::json::load(req.body);
        if(!body){return crow::response(400, "invalid json");}

        Job job;
        job.client_id= body["client_id"].s();
        job.type= body["type"].s();
        job.attempts=0;
        job.max_retries=5;
        job.status= Status::PENDING;
        job.payload= json::parse(body["payload"].dump());
        
        queu.R_queue_push(job);
        db.insert_job(job);

        crow::json::wvalue::res;
        res["job_id"]= jobs.id;
        res["status"]= "pending";
        return crow::response(202, res);
    });
    

    app.port(8080).multithreaded().run();
}