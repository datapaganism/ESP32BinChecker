#pragma once

#include "Arduino.h"
#include "bin.hpp"


uint32_t seconds_to_midnight();

bool _is_today(struct tm to_test, int day_offset);

bool is_today(struct tm to_test);

bool is_tomorrow(struct tm to_test);

struct tm get_current_time();

void copy_time_to_last(Bin &bin);

void ignore_hr_min_sec(struct tm &info);


void convert_string_to_time(Bin &bin);