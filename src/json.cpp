#include "json.hpp"
#include "utils.hpp"

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