#pragma once

#include <Arduino.h>
#include "bin.hpp"
#include <heltec-eink-modules.h>


void draw_last_updated_batt();
void draw_main_screen(Bin &recycling_bin, Bin &food_bin, Bin &black_bin);
void draw_error_screen(String error_string);
