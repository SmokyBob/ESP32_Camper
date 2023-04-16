#pragma once
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

#ifdef OLED
enum oledPage
{
   SENSOR_PAGE,
   CONFIG_PAGE
};
#endif