#if BLE_APP
#ifndef BLE_SERVICE_h
#define BLE_SERVICE_h

#include "globals.h"
#include "NimBLEDevice.h"
#if defined(CAMPER)
#include "site.h"
#endif
#include "LoraUtils.h"

extern void initBLEService();

extern void handleBLE();

#endif
#endif