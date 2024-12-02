#include "globals.h"

keys_t data[15] = {
    {1, "TEMP", "Temperature", "", "", true, false},
    {2, "HUM", "Humidity", "", "", true, false},
    {3, "VOLTS", "Battery Voltage", "", "5b4c2c35-8a17-4d41-aec2-04a7dc1eaf91", true, false},
    {4, "MILLIS", "Millis", "", "", false, false},
    {5, "DATETIME", "Date and Time", "", "2cdc00e8-907c-4f63-a284-2be098f8ea52", false, false},
    {6, "B_WINDOW", "Window Open/Closed", "0", "4efa5b56-0426-42d7-857e-3ae3370b4a1d", false, true},
    {7, "B_FAN", "Fan On/Off", "0", "e8db3027-e095-435d-929c-f471669209c3", false, true},
    {8, "B_HEATER", "Heater On/Off", "0", "4d15f090-6175-4e3c-b076-6ae0f69b7117", false, true},
    {9, "EXT_TEMP", "External sensor Temperature", "", "226115b6-f631-4f82-b58d-b84487b55a64", true, false},
    {10, "EXT_HUM", "External sensor Humidity", "", "b95cdb8a-7ee4-48c6-a818-fd11e60881f4", true, false},
    {11, "AMB_TEMP", "Ambient Temperature", "", "", false, false},
    {12, "AMB_HUM", "Ambient Humidity", "", "", false, false},
    {13, "IR_TEMP", "IR sensor Temperature", "", "", false, false},
    {14, "HAND_VOLTS", "Handheld Battery Voltage", "", "", false, false},
    {15, "TIME", "Time", "", "", true, false},
};
// TODO: add week day value
keys_t *getDataObj(const char *key)
{
    keys_t *toRet;
    for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
    {
        if (strcmp(data[i].key, key) == 0)
        {
            toRet = &data[i];
            break;
        }
    }
    return toRet;
};

String getDataVal(const char *key)
{
    return getDataObj(key)->value;
};

void setDataVal(const char *key, const String value)
{
    for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
    {
        if (strcmp(data[i].key, key) == 0)
        {
            data[i].value = value;
            break;
            ;
        }
    }
}

keys_t config[12] = {
    {1, "SERVO_CL_POS", "Window Closed (servo degs)", "", "", false, false},
    {2, "SERVO_OP_POS", "Window OPEN (servo degs)", "", "", false, false},
    {3, "SERVO_CL_TEMP", "AUTOMATION: Window Closed (temp)", "", "", false, false}, // TODO: move to automation
    {4, "SERVO_OP_TEMP", "AUTOMATION: Window OPEN (temp)", "", "", false, false},   // TODO: move to automation
    {5, "VOLT_ACTUAL", "Current voltage for calibration", "", "", false, false},    // stored preference value is VDiv_Calibration
    {6, "VOLT_LIM", "Low Voltage (init sleep)", "", "", false, false},
    {7, "VOLT_LIM_UL", "Low Voltage UNDER LOAD (init sleep)", "", "", false, false},
    {8, "VOLT_LIM_SL_M", "Sleep time on Low Voltage (minutes)", "", "", false, false},
    {9, "B_VOLT_LIM_IGN", "Force Ignore Voltage Limits ON/OFF", "", "70c74d81-5a61-43c0-b82b-08fcc9109ff4", false, false},
    {10, "HEAT_ON_TEMP", "AUTOMATION: Turn HEATER ON (temp)", "", "", false, false},   // TODO: move to automation
    {11, "HEAT_OFF_TEMP", "AUTOMATION: Turn HEATER OFF (temp)", "", "", false, false}, // TODO: move to automation
    {12, "B_AUTOMATION", "Enable automation", "", "ea7614e2-7eb9-4e1c-8ac4-5e64c3994264", false, false},
};

keys_t *getConfigObj(const char *key)
{
    keys_t *toRet;
    for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
    {
        if (strcmp(config[i].key, key) == 0)
        {
            toRet = &config[i];
            break;
        }
    }
    return toRet;
};

String getConfigVal(const char *key)
{
    return getConfigObj(key)->value;
};

void setConfigVal(const char *key, const String value)
{
    for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
    {
        if (strcmp(config[i].key, key) == 0)
        {
            config[i].value = value;
            break;
        }
    }
};

automation_t **automationArray = NULL;

u_long last_Millis;
String last_IgnoreLowVolt;
String last_FanOn;

float last_SNR;
float last_RSSI;

batt_perc batt_perc_12_list[14] = {
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

batt_perc batt_perc_3_7_list[21] = {
    {4.2, 100},
    {4.15, 95},
    {4.11, 90},
    {4.08, 85},
    {4.02, 80},
    {3.98, 75},
    {3.95, 70},
    {3.91, 65},
    {3.87, 60},
    {3.85, 55},
    {3.84, 50},
    {3.82, 45},
    {3.8, 40},
    {3.79, 35},
    {3.77, 30},
    {3.75, 25},
    {3.73, 20},
    {3.71, 15},
    {3.69, 10},
    {3.61, 5},
    {3.27, 0}};

unsigned long lastLORASend = 0;

#if defined(CAMPER) || defined(EXT_SENSORS)
// Redefine default value for MCU without these values, just to keep settings consistents on init
#ifndef VDiv_Calibration
#define VDiv_Calibration 1.0555
#endif
#ifndef Servo_closed_pos
#define Servo_closed_pos 40
#endif
#ifndef Servo_OPEN_pos
#define Servo_OPEN_pos 175
#endif

Preferences prf_config;

void loadPreferences()
{
    prf_config.begin("CAMPER", false);

    setConfigVal("SERVO_CL_POS", prf_config.getString("SERVO_CL_POS", String(Servo_closed_pos)));
    setConfigVal("SERVO_OP_POS", prf_config.getString("SERVO_OP_POS", String(Servo_OPEN_pos)));
    setConfigVal("SERVO_CL_TEMP", prf_config.getString("SERVO_CL_TEMP", String(20)));
    setConfigVal("SERVO_OP_TEMP", prf_config.getString("SERVO_OP_TEMP", String(30)));
    setConfigVal("VOLT_ACTUAL", prf_config.getString("VOLT_ACTUAL", String(VDiv_Calibration)));

    setConfigVal("VOLT_LIM", prf_config.getString("VOLT_LIM", String(12.00)));
    setConfigVal("VOLT_LIM_UL", prf_config.getString("VOLT_LIM_UL", String(11.40)));
    setConfigVal("VOLT_LIM_SL_M", prf_config.getString("VOLT_LIM_SL_M", String(30)));

    setConfigVal("B_VOLT_LIM_IGN", prf_config.getString("B_VOLT_LIM_IGN", String(0)));

    // TODO: move to automation
    setConfigVal("HEAT_ON_TEMP", prf_config.getString("HEAT_ON_TEMP", String(5.00)));
    setConfigVal("HEAT_OFF_TEMP", prf_config.getString("HEAT_OFF_TEMP", String(10.00)));

    setConfigVal("B_AUTOMATION", prf_config.getString("B_AUTOMATION", String(0)));

    String last_DateTime = prf_config.getString("lastTime", "");

    prf_config.end();
    if (last_DateTime.compareTo("") != 0)
    {
        setDateTime(last_DateTime);
    }

    savePreferences();
};

void savePreferences()
{

    prf_config.begin("CAMPER", false);

    for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
    {
        prf_config.putString(config[i].key, config[i].value);
    }

    prf_config.putString("lastTime", getDataVal("DATETIME"));

    prf_config.end();
};

void resetPreferences()
{
    setConfigVal("SERVO_CL_POS", String(Servo_closed_pos));
    setConfigVal("SERVO_OP_POS", String(Servo_OPEN_pos));
    setConfigVal("SERVO_CL_TEMP", String(20));
    setConfigVal("SERVO_OP_TEMP", String(30));
    setConfigVal("VOLT_ACTUAL", String(VDiv_Calibration));

    setConfigVal("VOLT_LIM", String(12.00));
    setConfigVal("VOLT_LIM_UL", String(11.40));
    setConfigVal("VOLT_LIM_SL_M", String(30));

    // TODO: move to automation
    setConfigVal("HEAT_ON_TEMP", String(5.00));
    setConfigVal("HEAT_OFF_TEMP", String(10.00));

    setConfigVal("B_AUTOMATION", String(0));

    // Full preferences cleanup
    prf_config.begin("CAMPER", false);
    prf_config.clear();
    prf_config.end();

    savePreferences();
};

// TODO: load automation from Preferences
//       "auto_0" contains the number of preferences
//       init the dinamic array
//       automationArray = (automation_t **)calloc(X, sizeof(automation_t *));
//       load and save automation

#endif

void setDateTime(String utcString)
{
    // Read in timezone of format 2023-03-14T00:00:00.000Z
    struct tm tm;
    strptime(utcString.c_str(), "%FT%T", &tm);

    time_t t = mktime(&tm);
    char buf[100];
    strftime(buf, sizeof(buf), "%FT%T", &tm);
    Serial.printf("Setting DateTime: %s \n", buf);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
#if defined(CAMPER) || defined(EXT_SENSORS)
    prf_config.begin("CAMPER", false);
    prf_config.putString("lastTime", utcString);

    prf_config.end();
#endif
    setDataVal("DATETIME", utcString);
    // TODO: update data[X].key== "TIME" with current time value
};

#if defined(CAMPER)
String EXT_SENSORS_URL = "";
#endif
#if defined(EXT_SENSORS)
String CAMPER_URL = "http://esp32-CAMPER.local";
#endif

bool clientConnected = false;