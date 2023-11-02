#include "globals.h"

u_long last_Millis;
float last_Temperature = NAN;
float last_Humidity = NAN;
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

#if defined(CAMPER) || defined(EXT_SENSORS)
//Redefine default value for MCU without these values, just to keep settings consistents on init
#ifndef VDiv_Calibration
#define VDiv_Calibration 1.0555
#endif
#ifndef Servo_closed_pos
#define Servo_closed_pos 40
#endif
#ifndef Servo_OPEN_pos
#define Servo_OPEN_pos 175
#endif
setting settings[9]{
    {"ServoPosClosed", "AUTOMATION: Window Closed (servo degs)", Servo_closed_pos},
    {"ServoPosOpen", "AUTOMATION: Window OPEN (servo degs)", Servo_OPEN_pos},
    {"ServoTempClosed", "AUTOMATION: Window Closed (temp)", 20},
    {"ServoTempOpen", "AUTOMATION: Window OPEN (temp)", 30},
    {"VDiv_Calib", "Current voltage for calibration", VDiv_Calibration},
    {"voltageLimit", "Low Voltage (init sleep)", 12.00},
    {"voltageLimit_underLoad", "Low Voltage UNDER LOAD (init sleep)", 11.40},
    {"lowVoltSleepMin", "Sleep time on Low Voltage (minutes)", 30.00},
    {"bAutomation", "Enable automation", 0.00} // default false
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
    settings[6].value = prf_config.getFloat("voltageLimit_underLoad", 11.40);
    settings[7].value = prf_config.getFloat("lowVoltSleepMin", 30.00);
    settings[8].value = prf_config.getFloat("bAutomation", 0.00);

    prf_config.end();
    savePreferences();
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
    settings[5].value = 11.40;
    settings[7].value = 30.00;
    settings[8].value = 0.00;

    savePreferences();
};

#endif

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

#if defined(CAMPER) 
String EXT_SENSORS_URL = "";
#endif
#if defined(EXT_SENSORS)
String CAMPER_URL = "http://esp32-CAMPER.local";
#endif