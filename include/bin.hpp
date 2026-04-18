#pragma once

#include <Arduino.h>
#include <time.h>

class Bin {

public:
    String time_str;
    struct tm time_info;
    const char* pretty_name;
    const char* key_name;

    Bin(const char* key_name, const char* pretty_name); 
};