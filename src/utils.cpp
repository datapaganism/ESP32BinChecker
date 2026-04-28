#include "utils.hpp"

uint32_t seconds_to_midnight()
{
    time_t now;
    time_t midnight;
    struct tm to_midnight;
    double diff;

    now = time(nullptr);
    localtime_r(&now, &to_midnight);

    to_midnight.tm_hour = 0;
    to_midnight.tm_min = 0;
    to_midnight.tm_sec = 0;
    to_midnight.tm_mday += 1;

    midnight = mktime(&to_midnight);
    diff = difftime(midnight, now);
    return uint32_t(diff);
}

bool _is_today(struct tm to_test, int day_offset)
{
    struct tm now = get_current_time();
    ignore_hr_min_sec(now);
    ignore_hr_min_sec(to_test);

    now.tm_mday += day_offset;

    time_t now_s = mktime(&now);
    time_t to_test_s = mktime(&to_test);

    double diff = difftime(to_test_s, now_s);

    return (-(60 * 60 * 24) < diff && diff < (60 * 60 * 24));
}

bool is_today(struct tm to_test)
{
    return _is_today(to_test, 0);
}

bool is_tomorrow(struct tm to_test)
{
    return _is_today(to_test, 1);
}

struct tm get_current_time()
{
    time_t now_secs = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now_secs, &timeinfo);

    return timeinfo;
}

void copy_time_to_last(Bin &bin)
{
    bin.time_info_last = bin.time_info;
}

void ignore_hr_min_sec(struct tm &info)
{
    info.tm_hour = 0;
    info.tm_min = 0;
    info.tm_sec = 0;
}

void convert_string_to_time(Bin &bin)
{
    copy_time_to_last(bin);

    strptime(bin.time_str.c_str(), "%Y-%m-%dT%T%z", &bin.time_info);
    // We don't care about the hour
    ignore_hr_min_sec(bin.time_info);
}