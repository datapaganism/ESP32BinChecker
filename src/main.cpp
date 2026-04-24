#include <Arduino.h>

#include <https.hpp>
#include <bin.hpp>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <NetworkClientSecure.h>
#include <ArduinoJson.h>
#include "esp_sntp.h"

// #undef DISPLAY
#ifdef WIRELESS_PAPER
#define DISPLAY true
#endif

#if (DISPLAY)
#include <heltec-eink-modules.h>
// #include "Fonts/FreeMono9pt7b.h"
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

// Having issues with day offsets
const char *ansii_days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char *days[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

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

bool is_(JsonObject input, const char *key, const char *value)
{
  if (!key || !value)
  {
    Serial.println(key);
    Serial.println(value);
    return false;
  }

  return (strncmp(input[key], value, strlen(value)) == 0);
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

void parseJson(JsonDocument doc, Bin &bin)
{
  for (JsonObject item : doc["value"].as<JsonArray>())
  {
    if (!is_(item, "hso_servicename", bin.key_name))
    {
      continue;
    }

    bin.time_str = String(item["hso_nextcollection"]);
    convert_string_to_time(bin);
  }
}

Bin black_bin("Refuse", "Black");
Bin recycling_bin("Recycling", "Recycling");
Bin food_bin("Food", "Food");

struct tm get_current_time()
{
  time_t now_secs = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now_secs, &timeinfo);

  return timeinfo;
}

bool _is_today(struct tm to_test, int day_offset)
{
  struct tm now = get_current_time();
  ignore_hr_min_sec(now);
  ignore_hr_min_sec(to_test);

  now.tm_mday += day_offset;

  time_t now_s = mktime(&now);
  time_t to_test_s = mktime(&to_test);

  double diff = difftime(to_test_s,now_s);

  return (-(60 * 60 * 24) < diff && diff < (60 * 60 * 24));
}

bool is_today(struct tm to_test)
{
  return _is_today(to_test,0);
}

bool is_tomorrow(struct tm to_test)
{
  return _is_today(to_test,1);
}

#if (DISPLAY)

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

    display.setCursor(0, 75);
    display.print(black);

    strftime((char *)&time_buf, sizeof(time_buf), "%d %b", &black_bin.time_info);
    snprintf(line_buf, sizeof(line_buf), " %s %s", days[black_bin.time_info.tm_wday], time_buf);

    display.setCursor(0, 92);
    if (is_tomorrow(black_bin.time_info))
    {
      display.print("Tomorrow");
    }
    else if (is_today(black_bin.time_info))
    {
      display.print("Today");
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
#endif

uint32_t seconds_to_midnight()
{
  time_t now;
  time_t midnight;
  struct tm to_midnight;
  double diff;

  now = time(NULL);
  to_midnight = *localtime(&now);

  to_midnight.tm_hour = 0;
  to_midnight.tm_min = 0;
  to_midnight.tm_sec = 0;
  to_midnight.tm_mday += 1;

  midnight = mktime(&to_midnight);
  diff = difftime(midnight, now);
  return uint32_t(diff);
}

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
    delay(60*60*24*1000);
  }

  error = deserializeJson(doc, rawData);
  if (error)
  {
    Serial.printf("deserializeJson() failed: %s\n", error.f_str());
    draw_error_screen("deserializeJson() failed");
    delay(60*60*24*1000);
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
