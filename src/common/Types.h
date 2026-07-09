#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <string>
#include "nlohmann/json.hpp"
#include <chrono>
#include "hiredis/hiredis.h" 


using JobId= uint64_t;
using json= nlohmann:: json;
using Priority= uint8_t;


using Time= std::chrono::system_clock::time_point;
enum class Status{
    PENDING,
    PROCESSING,
    DONE,
    FAILED,
    DEAD
};

#endif