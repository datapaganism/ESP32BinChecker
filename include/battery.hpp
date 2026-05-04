#include <Arduino.h>

#define LOW_BAT_VOLTAGE 3.0
#define FULL_BAT_VOLTAGE 4.2
#define LOW_BAT_SLEEP_S 100000

void battery_gpio_setup();

float get_battery_voltage();

float get_battery_pc();