#include <Arduino.h>
// Configuration
#include <config.h>
// copy src/config_example.h to src/credentials.h and put your data to the file

#include <WiFi.h>
#include <BlynkSimpleEsp32.h> // https://github.com/blynkkk/blynk-library
#include <WidgetRTC.h>
#include <TimeLib.h> // https://github.com/PaulStoffregen/Time
#include <Ticker.h>  // https://github.com/esp8266/Arduino/blob/master/libraries/Ticker
                     // https://github.com/esp8266/Arduino/blob/master/libraries/Ticker

// Sensors
#include <Wire.h>
#include <Adafruit_BME280.h>

#define DEBUG_SERIAL Serial

// Config
const char device_id[] = DEVICE_ID;
const char fw_ver[] = FW_VERSION;

const char timerOutPin = TIMER_OUTPUT_PIN;
const char coolerOutPin = COOLER_OUTPUT_PIN;

bool shouldSendData{false};
Ticker dataSender;

// Timer
uint8_t outControlType{1}; // 1 - auto, 2 - manual
uint16_t outManualValue{0};
bool shouldCheckTimer{false};
bool disableTimer{false};
Ticker timerChecker;
WidgetRTC rtc;
time_t startTime{7 * 3600};
time_t stopTime{19 * 3600};
int32_t tzOffset{3600};

// Sensors
Adafruit_BME280 bme;

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
        Blynk.virtualWrite(V2, airPressure);
        DEBUG_SERIAL.print("Air Pressure (hPa): ");
        DEBUG_SERIAL.println(airPressure);

        shouldSendData = false;
}

void checkTimer()
{
        uint16_t desiredTimerOutState{0};
        if (outControlType == 1 && disableTimer == false)
        {
                time_t currentTime{now() % 86400 + tzOffset};

                if (startTime > stopTime)
                {
                        if (currentTime > startTime)
                        {
                                desiredTimerOutState = outManualValue;
                        }
                }
                else
                {
                        if (currentTime > startTime && currentTime < stopTime)
                        {
                                desiredTimerOutState = outManualValue;
                        }
                }
        }
        else
        {
                if (outControlType == 2)
                {
                        desiredTimerOutState = outManualValue;
                }
        }

        sigmaDeltaWrite(0, desiredTimerOutState);
        digitalWrite(coolerOutPin, desiredTimerOutState > COOLING_THRESHOLD);

        shouldCheckTimer = false;
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

        // Timer controlled output
        pinMode(timerOutPin, OUTPUT);
        sigmaDeltaSetup(0, 1220);
        //attach pin 18 to channel 0
        sigmaDeltaAttachPin(timerOutPin, 0);
        //initialize channel 0 to off
        sigmaDeltaWrite(0, 0);
        pinMode(coolerOutPin, OUTPUT);
        digitalWrite(coolerOutPin, 0);

        // Extra output
        pinMode(EXTRA_OUTPUT, OUTPUT);
        digitalWrite(EXTRA_OUTPUT, HIGH);

        if (bme.begin(BME280_ADDR))
        {
                dataSender.attach(5.0, [] { shouldSendData = true; });
        }
        else
        {
                DEBUG_SERIAL.println("BME280 not found");
        }

        timerChecker.attach(1.0, [] { shouldCheckTimer = true; });
}

void loop()
{
        Blynk.run();
        if (shouldSendData)
        {
                sendData();
        }

        if (shouldCheckTimer)
        {
                checkTimer();
        }
}

// Sync state on reconnect
BLYNK_CONNECTED()
{
        Blynk.syncAll();
        rtc.begin();
}

// Timer
BLYNK_WRITE(V10)
{
        TimeInputParam t(param);

        if (t.hasStartTime())
        {
                startTime = t.getStart().getUnixOffset();
        }

        if (t.hasStopTime())
        {
                stopTime = t.getStop().getUnixOffset();
        }

        disableTimer = !(t.hasStartTime() && t.hasStopTime());

        tzOffset = t.getTZ_Offset();
}

BLYNK_WRITE(V11)
{
        outControlType = param.asInt();
}

BLYNK_WRITE(V12)
{
        outManualValue = param.asInt();
}
