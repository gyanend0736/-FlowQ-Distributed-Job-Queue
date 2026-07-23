#include "Priority.h"
#include<iostream>
#include<string>

Priority priority_from_string(const std::string& str){
    std::string res= str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if(res=="high") return 1;
    else if(res=="med") return 2;
    else return 3;
}