#include "display.hpp"

#include "utils.hpp"

#include <heltec-eink-modules.h>
// #include "Fonts/FreeMono9pt7b.h"
extern EInkDisplay_WirelessPaperV1_2 display;


#if (DISPLAY)

// Having issues with day offsets
const char *ansii_days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char *days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

void draw_last_updated_batt()
{
    char time_buf[25];
    char buf[25];

    int bat = 100;

    struct tm time_now = get_current_time();
    strftime((char *)&time_buf, sizeof(time_buf), "%T", &time_now);
    sniprintf(buf, sizeof(buf), "LU:%s BAT:%i% ", time_buf, bat);

    display.setTextSize(1);
    display.setCursor(0, 115);
    display.print(buf);
}

void draw_main_screen(Bin &recycling_bin, Bin &food_bin, Bin &black_bin)
{
    char line_buf[25];
    char time_buf[25];

    const char *next_bin_collection = "Next bin collection";
    const char *recycling_and_food = "Recycling & Food";
    const char *black = "Black";
    const char *everything = "Everything";
    bool recycle_food_same = (memcmp(&recycling_bin.time_info, &food_bin.time_info, sizeof(tm)) == 0);
    bool recycle_black_same = (memcmp(&recycling_bin.time_info, &black_bin.time_info, sizeof(tm)) == 0);

    struct tm time_now = get_current_time();
    strftime((char *)&time_buf, sizeof(time_buf), "%d %b %Y", &time_now);
    snprintf(line_buf, sizeof(line_buf), "%s %s", ansii_days[time_now.tm_wday], time_buf);

    display.clearMemory();
    display.setTextSize(2);
    display.setTextWrap(false);

    display.setCursor(display.getTextCenterX(line_buf), 0);
    display.print(line_buf);

    display.setCursor(display.getTextCenterX(next_bin_collection), 17);
    display.print(next_bin_collection);

    if (recycle_food_same && recycle_black_same)
    {
        display.setCursor(0, 40);
        display.print(everything);

        strftime((char *)&time_buf, sizeof(time_buf), "%d %b", &recycling_bin.time_info);
        snprintf(line_buf, sizeof(line_buf), " %s %s", days[recycling_bin.time_info.tm_wday], time_buf);

        display.setCursor(0, 57);

        if (is_tomorrow(recycling_bin.time_info))
        {
            display.print("Tomorrow");
        }
        else if (is_today(recycling_bin.time_info))
        {
            display.print("Today");
        }
        else
        {
            display.print(line_buf);
        }
    }
    else
    {
        display.setCursor(0, 40);
        display.print(recycling_and_food);

        strftime((char *)&time_buf, sizeof(time_buf), "%d %b", &recycling_bin.time_info);
        snprintf(line_buf, sizeof(line_buf), " %s %s", days[recycling_bin.time_info.tm_wday], time_buf);

        display.setCursor(0, 57);
        if (is_tomorrow(recycling_bin.time_info))
        {
            display.print(" Tomorrow");
        }
        else if (is_today(recycling_bin.time_info))
        {
            display.print(" Today");
        }
        else
        {
            display.print(line_buf);
        }

        display.setCursor(0, 75);
        display.print(black);

        strftime((char *)&time_buf, sizeof(time_buf), "%d %b", &black_bin.time_info);
        snprintf(line_buf, sizeof(line_buf), " %s %s", days[black_bin.time_info.tm_wday], time_buf);

        display.setCursor(0, 92);
        if (is_tomorrow(black_bin.time_info))
        {
            display.print(" Tomorrow");
        }
        else if (is_today(black_bin.time_info))
        {
            display.print(" Today");
        }
        else
        {
            display.print(line_buf);
        }
    }

    display.drawLine(1, 34, 250, 34, 0);

    draw_last_updated_batt();

    display.update();
}

void draw_error_screen(String error_string)
{
    display.clearMemory();
    display.setTextSize(2);
    display.setTextWrap(true);
    display.setCursor(0, 0);
    display.print("ERROR :(");

    display.setCursor(0, 17);
    display.print(error_string.c_str());

    draw_last_updated_batt();

    display.update();
}
#include <ArduinoJson.h>
#endif