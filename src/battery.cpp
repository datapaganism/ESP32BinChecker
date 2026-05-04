#include "battery.hpp"

#define ADC_CTRL 19
#define BATTERY_PIN 20


void battery_gpio_setup()
{
    pinMode(ADC_CTRL,OUTPUT);
    digitalWrite(ADC_CTRL, LOW);

    pinMode(BATTERY_PIN,INPUT);
}

float get_battery_voltage()
{
    size_t itrs = 10;
    float millivolts = 0;

    for (size_t i = 0; i < itrs; i++)
    {
        millivolts += (float)analogReadMilliVolts(BATTERY_PIN);
        delay(100);
    }

    millivolts /= itrs;

    // The voltage divider gives you half, so double it and turn to volts
    return ((millivolts * 2) / 1000);
}

float get_battery_pc()
{
    float voltage = get_battery_voltage();
    voltage = ((voltage - LOW_BAT_VOLTAGE) / (FULL_BAT_VOLTAGE - LOW_BAT_VOLTAGE)) * 100;
    return (voltage > 100) ? 100 : voltage; 
}