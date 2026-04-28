#include "https.hpp"
#include "cert.hpp"

int getData(String &out)
{
    int ret = -1;
    NetworkClientSecure *client = new NetworkClientSecure;
    if (!client)
    {
        Serial.println("Unable to create client");
        delete client;
        return ret;
    }

    client->setCACert(rootCACertificate);
    {
        // Add a scoping block for HTTPClient https to make sure it is destroyed before NetworkClientSecure *client is
        HTTPClient https;

        ret = https.begin(*client, API_URL);
        if (ret)
        { // HTTPS
            // start connection and send HTTP header
            int httpCode = https.GET();

            // httpCode will be negative on error
            if (httpCode > 0)
            {
                // HTTP header has been send and Server response header has been handled

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    out = https.getString();
                    ret = 0;
                }
            }
            else
            {
                Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
            }

            https.end();
        }
        else
        {
            Serial.printf("[HTTPS] Unable to connect\n");
        }

        // End extra scoping block
    }

    delete client;
    return ret;
}