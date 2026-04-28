#include <Arduino.h>

#include <https.hpp>
#include <bin.hpp>
#include <utils.hpp>
#include <json.hpp>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <NetworkClientSecure.h>
#include "esp_sntp.h"

#define TZ "GMT0BST,M3.5.0/1,M10.5.0"
#define _WIFI_TIMEOUT_S 10
#define _CLOCK_TIMEOUT_S 120

#define WIFI_TIMEOUT (_WIFI_TIMEOUT_S * 1000)
#define CLOCK_TIMEOUT (_CLOCK_TIMEOUT_S * 1000)
#define ERROR_SLEEP_S 30


// #undef DISPLAY
#ifdef WIRELESS_PAPER
#define DISPLAY true
#endif

#if (DISPLAY)
#include "display.hpp"
EInkDisplay_WirelessPaperV1_2 display;
#endif

#define FONT_SIZE 2
#define CHAR_WIDTH (5 * FONT_SIZE)
#define CHAR_HEIGHT (7 * FONT_SIZE)
#define SCREEN_WIDTH 250
#define SCREEN_HEIGHT 122

#define DRYRUN false
#if (DRYRUN)
// https://tomeko.net/online_tools/cpp_text_escape.php?lang=en
// Removed potentially doxxable information
const char *TESTJSON = "{\"@odata.context\":\"https://sgc-cp-prod.crm11.dynamics.com/api/data/v9.2/$metadata#hso_virtualserviceinformations\",\"value\":[{\"hso_statesource\":\"RoundLegInstance\",\"hso_roundleg\":\"XXX\",\"hso_uprn\":\"XXX\",\"hso_serviceid\":144,\"hso_reason\":\"Outside time allowed\",\"hso_resolutionname\":null,\"hso_subscriptionenddate\":null,\"hso_nextcollection\":\"2026-04-13T00:00:00+01:00\",\"hso_servicename\":\"Refuse\",\"hso_lastcollection\":\"2026-03-30T07:00:00+01:00\",\"hso_scheduledescription\":\"Monday every other week\",\"hso_name\":\"XXX\",\"hso_nextcollectiondate\":null,\"hso_missedcollectionallowed\":false,\"hso_lastcollectiondate\":null,\"hso_resolutionsource\":null,\"hso_lastcollectioncompleted\":\"2026-03-30T11:49:39+01:00\",\"hso_round\":\"CR 07 Mon B\",\"hso_statename\":\"Closed Completed\",\"hso_lastcollectioncompleteddate\":null,\"hso_virtualserviceinformationid\":\"00000000-0000-0000-0000-000XXX144\",\"hso_roundgroup\":\"Refuse CR 07\"},{\"hso_statesource\":\"Task\",\"hso_roundleg\":\"XXX\",\"hso_uprn\":\"XXX\",\"hso_serviceid\":146,\"hso_reason\":\"Unable to complete - container / Item not presented\",\"hso_resolutionname\":\"Bin / Item Not Out\",\"hso_subscriptionenddate\":null,\"hso_nextcollection\":\"2026-04-13T07:00:00+01:00\",\"hso_servicename\":\"Recycling\",\"hso_lastcollection\":\"2026-04-06T07:00:00+01:00\",\"hso_scheduledescription\":\"Monday every week\",\"hso_name\":\"XXX\",\"hso_nextcollectiondate\":null,\"hso_missedcollectionallowed\":false,\"hso_lastcollectiondate\":null,\"hso_resolutionsource\":\"Task\",\"hso_lastcollectioncompleted\":\"2026-04-06T09:03:31+01:00\",\"hso_round\":\"CK 18 Mon\",\"hso_statename\":\"Closed Not Completed\",\"hso_lastcollectioncompleteddate\":null,\"hso_virtualserviceinformationid\":\"00000000-0000-0000-0000-000XXX146\",\"hso_roundgroup\":\"Recycling CK 18\"},{\"hso_statesource\":\"Task\",\"hso_roundleg\":\"XXX\",\"hso_uprn\":\"XXX\",\"hso_serviceid\":148,\"hso_reason\":\"Unable to complete - container / Item not presented\",\"hso_resolutionname\":\"Bin / Item Not Out\",\"hso_subscriptionenddate\":null,\"hso_nextcollection\":\"2026-04-13T07:00:00+01:00\",\"hso_servicename\":\"Food\",\"hso_lastcollection\":\"2026-04-06T07:00:00+01:00\",\"hso_scheduledescription\":\"Monday every week\",\"hso_name\":\"XXX\",\"hso_nextcollectiondate\":null,\"hso_missedcollectionallowed\":false,\"hso_lastcollectiondate\":null,\"hso_resolutionsource\":\"Task\",\"hso_lastcollectioncompleted\":\"2026-04-06T09:03:39+01:00\",\"hso_round\":\"CK 18 Mon\",\"hso_statename\":\"Closed Not Completed\",\"hso_lastcollectioncompleteddate\":null,\"hso_virtualserviceinformationid\":\"00000000-0000-0000-0000-000XXX148\",\"hso_roundgroup\":\"Recycling CK 18\"}]}";
#endif


WiFiMulti WiFiMulti;
Bin black_bin("Refuse", "Black");
Bin recycling_bin("Recycling", "Recycling");
Bin food_bin("Food", "Food");

int setup_clock()
{
    configTzTime(TZ, "pool.ntp.org");
    const uint64_t timeout = millis() + CLOCK_TIMEOUT;

    Serial.print(F("Waiting for NTP time sync: "));

    sntp_sync_status_t status = sntp_get_sync_status();

    if ((status == SNTP_SYNC_STATUS_RESET))
    {
        while ((status == SNTP_SYNC_STATUS_RESET) && (millis() < timeout))
        {
            status = sntp_get_sync_status();
            Serial.print(".");
            delay(100);
        }
    }

    if (status == SNTP_SYNC_STATUS_RESET)
    {
        return -1;
    }
    
    Serial.println(" synced");
    return 0;
}


int setup_wifi()
{
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PSK);

    const uint64_t timeout = millis() + WIFI_TIMEOUT;

    // wait for WiFi connection
    Serial.print("Waiting for Wifi connection: ");
    while ((WiFiMulti.run() != WL_CONNECTED) || millis() < timeout)
    {
        delay(100);
        Serial.print(".");
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Wifi Timeout");
        return -1;
    }

    Serial.println(" connected");
    return 0;
}

void turn_off_wifi()
{
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void turn_off_clock()
{
    esp_sntp_stop();
}

void enter_deep_sleep(uint64_t time_s, uint64_t ms_offset = 0)
{
    time_s *= 1000000ULL;
    ms_offset *= 1000ULL;

    if (ms_offset > time_s)
    {
        ms_offset = 0;
    }

    esp_sleep_enable_timer_wakeup((uint64_t)time_s - ms_offset);
    Serial.printf("Sleeping for %is\n", (time_s - ms_offset) / 1000000ULL);
    esp_deep_sleep_start();
}



static void on_error(String error_msg)
{
    init_display();
    turn_off_clock();
    turn_off_wifi();
    draw_error_screen(error_msg);
    Serial.println(error_msg);
    turn_off_display();
    enter_deep_sleep(ERROR_SLEEP_S);
}

void setup()
{
    Serial.begin(115200);

    uint64_t boot_up_time_ms = millis();

    int wifi_ret = setup_wifi();
    if (wifi_ret != 0)
    {
        on_error("Wifi failed to connect");
    }

    int clock_ret = setup_clock();
    if (clock_ret != 0)
    {
        on_error("Clock failed to sync");
    }

    int ret = 0;
    String rawData;
    JsonDocument doc;
    DeserializationError error;
    time_t nowSecs;
    struct tm timeinfo;
    char time_buf[26];
    char *output;
    char out_buf[200];
    size_t out_buf_index = 0;

    ret = getData(rawData);
    if (ret != 0)
    {
        on_error("Failed to get data from API");
    }

    turn_off_wifi();

    error = deserializeJson(doc, rawData);
    if (error)
    {
        on_error("Failed to deserialise json");
    }

    parseJson(doc, black_bin);
    parseJson(doc, recycling_bin);
    parseJson(doc, food_bin);

    init_display();
    draw_main_screen(recycling_bin, food_bin, black_bin);
    turn_off_display();

    // We have Wifi and NTP sync now
    boot_up_time_ms = millis() - boot_up_time_ms;
    Serial.printf("boot up time %i\n", boot_up_time_ms);

    uint32_t to_midnight = seconds_to_midnight();
    Serial.printf("Next refresh in %is\n", to_midnight);
    enter_deep_sleep(to_midnight);
}

void loop()
{

}
