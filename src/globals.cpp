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