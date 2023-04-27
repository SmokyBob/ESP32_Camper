#ifndef GLOBALS_H
#define GLOBALS_H
#include "Arduino.h"

enum data
{
   TEMPERATURE,
   HUMIDITY,
   VOLTS,
   MILLIS,
   DATETIME,//Format: yyyyMMddHHmmss
   WINDOW,//Controls the servo: 0 = open, 1 = closed
   RELAY1,
   RELAY2
};
enum dataType{
   DATA,
   COMMAND
};

#ifdef SENSORS
static float temperature = 0;
static float humidity = 0;
static float voltage = 0;
#endif

// TODO: array of values with same index as enums.data ?
static u_long last_Millis;
static float last_Temperature = 0;
static float last_Humidity = 0;
static float last_Voltage = 0;
static bool last_WINDOW;
static bool last_Relay1;
static bool last_Relay2;
static String last_DateTime;

static float last_SNR;
static float last_RSSI;
#endif