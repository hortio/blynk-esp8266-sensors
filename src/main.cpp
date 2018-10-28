#include <Arduino.h>
// Configuration
#include <credentials.h>
// copy src/credentials_example.h to src/credentials.h and put your data to the file

#include <ESP8266WiFi.h>            // https://github.com/esp8266/Arduino
#include <BlynkSimpleEsp8266_SSL.h> // https://github.com/blynkkk/blynk-library
#include <ESP8266httpUpdate.h>      // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266httpUpdate
#include <Ticker.h>                 // https://github.com/esp8266/Arduino/blob/master/libraries/Ticker

#define DEBUG_SERIAL Serial

// Config
const char device_id[] = DEVICE_ID;
const char fw_ver[] = FW_VERSION;

const char outPins[] = OUTPUT_PINS;

void setup()
{
        DEBUG_SERIAL.begin(115200);

        // Connections
        DEBUG_SERIAL.println("Connecting to WiFI");
        Blynk.connectWiFi(WIFI_SSID, WIFI_PASS);

        DEBUG_SERIAL.println("\nConnecting to Blynk server");
        Blynk.config(BLYNK_AUTH, BLYNK_SERVER, BLYNK_PORT);
        while (Blynk.connect() == false)
        {
                delay(500);
                DEBUG_SERIAL.print(".");
        }

        DEBUG_SERIAL.println("\nReady");

        // Devices
        for (uint8 i = 0; i <= 1; i++)
        {
                pinMode(outPins[i], OUTPUT);
                digitalWrite(outPins[i], LOW);
        }
}

void loop()
{
        Blynk.run();
}

// Sync state on reconnect
BLYNK_CONNECTED()
{
        Blynk.syncAll();
}

// Update FW
BLYNK_WRITE(V22)
{
        if (param.asInt() == 1)
        {
                DEBUG_SERIAL.println("FW update request");

                char full_version[34]{""};
                strcat(full_version, device_id);
                strcat(full_version, "::");
                strcat(full_version, fw_ver);

                t_httpUpdate_return ret = ESPhttpUpdate.update(FW_UPDATE_URL, full_version);
                switch (ret)
                {
                case HTTP_UPDATE_FAILED:
                        DEBUG_SERIAL.println("[update] Update failed.");
                        break;
                case HTTP_UPDATE_NO_UPDATES:
                        DEBUG_SERIAL.println("[update] Update no Update.");
                        break;
                case HTTP_UPDATE_OK:
                        DEBUG_SERIAL.println("[update] Update ok.");
                        break;
                }
        }
}
