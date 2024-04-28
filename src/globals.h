#ifndef GLOBALS_H
#define GLOBALS_H
#include "Arduino.h"
#if defined(CAMPER) || defined(EXT_SENSORS)
#include "Preferences.h"
#endif

struct keys_t
{
   int id;
   char key[15]; // 15 so that it can be used as preferences keys
   String description;
   String value;
   String ble_uuid;
   bool auto_condition;
   bool auto_action;
};

extern keys_t data[15]; // See .c file for details and to add values
keys_t *getDataObj(const char *key);
String getDataVal(const char *key);
void setDataVal(const char *key, const String value);

extern keys_t config[12]; // See .c file for details and to add values
keys_t *getConfigObj(const char *key);
String getConfigVal(const char *key);
void setConfigVal(const char *key, const String value);

enum automation_conditions
{
   AC_EQ,
   AC_NEQ,
   AC_LET,
   AC_GET
};
enum automation_action
{
   AC_TRUE,
   AC_FALSE,
   AC_OPEN,
   AC_CLOSE
};

struct automation_t
{
   int order;
   bool enabled;
   int data_id;
   automation_conditions condition;
   String value;
   int action_id;
   automation_action action;
};

extern automation_t **automationArray;

enum dataType
{
   DATA,
   CONFIGS
};

extern u_long last_Millis;
extern String last_IgnoreLowVolt;

extern float last_SNR;
extern float last_RSSI;

struct batt_perc
{
   float voltage;
   uint8_t percentage;
};

extern batt_perc batt_perc_12_list[14];
extern batt_perc batt_perc_3_7_list[21];

extern unsigned long lastLORASend;

#if defined(CAMPER) || defined(EXT_SENSORS)

void loadPreferences();
void savePreferences();
void resetPreferences();

#endif
void setDateTime(String utcString);

#if defined(CAMPER)
extern String EXT_SENSORS_URL;
#endif
#if defined(EXT_SENSORS)
extern String CAMPER_URL;
#endif
#endif