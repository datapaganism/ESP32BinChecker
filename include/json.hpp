#pragma once

#include "Arduino.h"
#include <ArduinoJson.h>
#include "bin.hpp"

bool is_(JsonObject input, const char *key, const char *value);

void parseJson(JsonDocument doc, Bin &bin);