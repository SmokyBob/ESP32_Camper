#ifndef GLOBALS_H
#define GLOBALS_H
#include "Arduino.h"
#include "Preferences.h"

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
   EXT_HUMIDITY
};
enum dataType
{
   DATA,
   COMMAND
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

#endif