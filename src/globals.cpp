#include "globals.h"

u_long last_Millis;
float last_Temperature = 0;
float last_Humidity = 0;
float last_Voltage = 0;
bool last_WINDOW;
bool last_Relay1;
bool last_Relay2;
float last_Ext_Temperature = NAN;
float last_Ext_Humidity = NAN;

String last_DateTime;
float last_SNR;
float last_RSSI;

batt_perc batt_perc_list[14] = {
    {13.60, 100},
    {13.40, 99},
    {13.32, 90},
    {13.28, 80},
    {13.20, 70},
    {13.16, 60},
    {13.13, 50},
    {13.10, 40},
    {13.00, 30},
    {12.90, 20},
    {12.80, 17},
    {12.50, 14},
    {12.00, 9},
    {10.00, 0}};

unsigned long lastLORASend = 0;

#ifdef SENSORS
setting settings[7]{
    {"ServoPosClosed", Servo_closed_pos},
    {"ServoPosOpen", Servo_OPEN_pos},
    {"ServoTempClosed", 20},
    {"ServoTempOpen", 30},
    {"VDiv_Calib", VDiv_Calibration},
    {"voltageLimit", 12.00},
    {"lowVoltSleepMin", 30.00},
};

Preferences prf_config;

void loadPreferences()
{
    prf_config.begin("CAMPER", false);

    settings[0].value = prf_config.getFloat("ServoPosClosed", Servo_closed_pos);
    settings[1].value = prf_config.getFloat("ServoPosOpen", Servo_OPEN_pos);
    settings[2].value = prf_config.getFloat("ServoTempClosed", 20);
    settings[3].value = prf_config.getFloat("ServoTempOpen", 30);
    settings[4].value = prf_config.getFloat("VDiv_Calib", VDiv_Calibration);
    settings[5].value = prf_config.getFloat("voltageLimit", 12.00);
    settings[6].value = prf_config.getFloat("lowVoltSleepMin", 30.00);

    prf_config.end();
};

void savePreferences()
{

    prf_config.begin("CAMPER", false);

    for (size_t i = 0; i < (sizeof(settings) / sizeof(setting)); i++)
    {
        prf_config.putFloat(settings[i].name.c_str(), settings[i].value);
    }

    prf_config.end();
};

void resetPreferences()
{
    settings[0].value = Servo_closed_pos;
    settings[1].value = Servo_OPEN_pos;
    settings[2].value = 20;
    settings[3].value = 30;
    settings[4].value = VDiv_Calibration;
    settings[5].value = 12.00;
    settings[6].value = 30.00;

    savePreferences();
};

void setTime(String utcString)
{
    // Read in timezone of format 2023-03-14T00:00:00.000Z
    struct tm tm;
    strptime(utcString.c_str(), "%FT%T", &tm);

    time_t t = mktime(&tm);
    char buf[100];
    strftime(buf, sizeof(buf), "%FT%T", &tm);
    Serial.printf("Setting time: %s", buf);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
};
#endif