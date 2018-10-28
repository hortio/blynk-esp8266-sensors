#include <Arduino.h>
// Configuration
#include <credentials.h>
// copy src/credentials_example.h to src/credentials.h and put your data to the file

#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <BlynkSimpleEsp8266.h> // https://github.com/blynkkk/blynk-library
#include <ESP8266httpUpdate.h>  // https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266httpUpdate
#include <Ticker.h>             // https://github.com/esp8266/Arduino/blob/master/libraries/Ticker

// Sensors
#include <Wire.h>
#include <Adafruit_BME280.h>

#define DEBUG_SERIAL Serial

// Config
const char device_id[] = DEVICE_ID;
const char fw_ver[] = FW_VERSION;

const char outPins[] = OUTPUT_PINS;

bool shouldSendData{false};
Ticker dataSender;

// Sensors

Adafruit_BME280 bme;

void setSendDataFlag()
{
        shouldSendData = true;
}

void sendData()
{
        float airTemperature{0};
        airTemperature = bme.readTemperature();
        Blynk.virtualWrite(V0, airTemperature);
        DEBUG_SERIAL.print("Air Temperature (C): ");
        DEBUG_SERIAL.println(airTemperature);

        float airHumidity{0};
        airHumidity = bme.readHumidity();
        Blynk.virtualWrite(V1, airHumidity);
        DEBUG_SERIAL.print("Air Humidity (%): ");
        DEBUG_SERIAL.println(airHumidity);

        float airPressure{0};
        airPressure = bme.readPressure() / 100.0F;
        DEBUG_SERIAL.print("Air Pressure (hPa): ");
        DEBUG_SERIAL.println(airPressure);

        shouldSendData = false;
}

void scanI2C()
{
        byte error, address;
        int nDevices;

        DEBUG_SERIAL.println("Scanning...");

        nDevices = 0;
        for (address = 1; address < 127; address++)
        {
                // The i2c_scanner uses the return value of
                // the Write.endTransmisstion to see if
                // a device did acknowledge to the address.
                Wire.beginTransmission(address);
                error = Wire.endTransmission();

                if (error == 0)
                {
                        DEBUG_SERIAL.print("I2C device found at address 0x");
                        if (address < 16)
                                DEBUG_SERIAL.print("0");
                        DEBUG_SERIAL.print(address, HEX);
                        DEBUG_SERIAL.println("  !");

                        nDevices++;
                }
                else if (error == 4)
                {
                        DEBUG_SERIAL.print("Unknown error at address 0x");
                        if (address < 16)
                                DEBUG_SERIAL.print("0");
                        DEBUG_SERIAL.println(address, HEX);
                }
        }
        if (nDevices == 0)
                DEBUG_SERIAL.println("No I2C devices found\n");
        else
                DEBUG_SERIAL.println("done\n");
}

void setup()
{
        DEBUG_SERIAL.begin(115200);

        Wire.begin(I2C_SDA, I2C_SCL);
        scanI2C();

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

        if (!bme.begin(BME280_ADDR))
        {
                DEBUG_SERIAL.println("BME280 not found");
        }

        // Timers;
        dataSender.attach(5.0, setSendDataFlag);
}

void loop()
{
        Blynk.run();
        if (shouldSendData)
        {
                sendData();
        }
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
