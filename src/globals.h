#ifndef GLOBALS_H
#define GLOBALS_H
#include "Arduino.h"
#if defined(CAMPER) || defined(EXT_SENSORS)
#include "Preferences.h"
#endif

enum data
{
   TEMPERATURE,
   HUMIDITY,
   VOLTS,
   MILLIS,
   DATETIME, // Format: yyyyMMddHHmmss
   WINDOW,   // Controls the servo: 0 = open, 1 = closed
   RELAY1,
   RELAY2,
   EXT_TEMPERATURE,
   EXT_HUMIDITY,
   CONFIG_SERVO_CLOSED_POS,
   CONFIG_SERVO_OPEN_POS,
   CONFIG_SERVO_CLOSED_TEMP,
   CONFIG_SERVO_OPEN_TEMP,
   CONFIG_VOLTAGE_ACTUAL,
   CONFIG_VOLTAGE_LIMIT,
   CONFIG_VOLTAGE_LIMIT_UNDER_LOAD,
   CONFIG_VOLTAGE_SLEEP_MINUTES,
   CONFIG_ENABLE_AUTOMATION,
   //TODO: config fan on/off temps
   //TODO: config heater on/off temps
};
enum dataType
{
   DATA,
   COMMAND,
   CONFIGS
};

extern u_long last_Millis;
extern float last_Temperature;
extern float last_Humidity;
extern float last_Voltage;
extern bool last_WINDOW;
extern bool last_Relay1;
extern bool last_Relay2;
extern String last_DateTime;
extern float last_Ext_Temperature;
extern float last_Ext_Humidity;

extern float last_SNR;
extern float last_RSSI;

struct batt_perc
{
   float voltage;
   uint8_t percentage;
};

extern batt_perc batt_perc_list[14];
extern unsigned long lastLORASend;

#if defined(CAMPER) || defined(EXT_SENSORS)
struct setting
{
   String name;
   String description;
   float value;
};

extern setting settings[9];

void loadPreferences();
void savePreferences();
void resetPreferences();

#endif
void setTime(String utcString);

#if defined(CAMPER) 
extern String EXT_SENSORS_URL;
#endif
#if defined(EXT_SENSORS)
extern String CAMPER_URL;
#endif
#endif