#include <Arduino.h>

#include <https.hpp>
#include <bin.hpp>
#include <utils.hpp>
#include <json.hpp>


#include <WiFi.h>
#include <WiFiMulti.h>

#include <NetworkClientSecure.h>
#include "esp_sntp.h"

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


// Not sure if NetworkClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
void setClock()
{
    configTime(0, 0, "pool.ntp.org");

    Serial.print(F("Waiting for NTP time sync: "));
    time_t nowSecs = time(nullptr);
    while (nowSecs < 8 * 3600 * 2)
    {
        delay(500);
        Serial.print(F("."));
        yield();
        nowSecs = time(nullptr);
    }

    Serial.println();
    struct tm timeinfo;
    gmtime_r(&nowSecs, &timeinfo);
    Serial.print(F("Current time: "));
    char buf[26];
    Serial.print(asctime_r(&timeinfo, buf));
}

WiFiMulti WiFiMulti;

void setupWifi()
{
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PSK);

    // wait for WiFi connection
    Serial.print("Waiting for WiFi to connect...");
    while ((WiFiMulti.run() != WL_CONNECTED))
    {
        Serial.print(".");
    }
    Serial.println(" connected");
}

Bin black_bin("Refuse", "Black");
Bin recycling_bin("Recycling", "Recycling");
Bin food_bin("Food", "Food");

void setup()
{

#if (DISPLAY)
    // display.setFont( & FreeMono9pt7b );
    display.landscape();
    display.clearMemory();
    display.setTextSize(2);
#endif

    Serial.begin(115200);

    Serial.println();
    Serial.println();
    Serial.println();

#if (!DRYRUN)
#if (DISPLAY)
    display.print("Waiting for Wifi\n");
    display.update();
#endif
    setupWifi();

#if (DISPLAY)
    display.print("Waiting for NTP Sync\n");
    display.update();
#endif
    setClock();

#else
    Serial.println(("DRYRUN"));
#endif

#if (DISPLAY)
    display.setCursor(0, 0);
    display.clearMemory();
    display.update();
#endif
}

void loop()
{
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

    nowSecs = time(nullptr);
#if (!DRYRUN)
    ret = getData(rawData);
#else
    rawData = String(TESTJSON);
#endif

    if (ret != 0)
    {
        draw_error_screen("Failed to get data");
        delay(60 * 60 * 24 * 1000);
    }

    error = deserializeJson(doc, rawData);
    if (error)
    {
        Serial.printf("deserializeJson() failed: %s\n", error.f_str());
        draw_error_screen("deserializeJson() failed");
        delay(60 * 60 * 24 * 1000);
    }

    // Need to print errors to the display and do a retry

    parseJson(doc, black_bin);
    parseJson(doc, recycling_bin);
    parseJson(doc, food_bin);

#if (DISPLAY)
    draw_main_screen(recycling_bin, food_bin, black_bin);
#endif

    uint32_t to_midnight = seconds_to_midnight();
    Serial.printf("Next refresh in %is\n", to_midnight);

    // // Configure hardware for low-power
    // Platform::prepareToSleep();
    // WiFi.disconnect(true);
    // WiFi.mode(WIFI_OFF);
    // esp_sntp_stop();

    // // How long until restart
    // esp_sleep_enable_timer_wakeup( (uint64_t)to_midnight * 1000 );

    // // Sleep now
    // esp_deep_sleep_start();
    // // This doesn't work currently, basically just causes a reset instead of wakeup

    Serial.println();
    delay(to_midnight * 1000);
}
