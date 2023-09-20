#ifndef REMOTEXYUTILS_H
#define REMOTEXYUTILS_H
#ifdef BLE_APP
/*
   -- ESP32 Camper BLE --
   
   This source code of graphical user interface 
   has been generated automatically by RemoteXY editor.
   To compile this code using RemoteXY library 3.1.8 or later version 
   download by link http://remotexy.com/en/library/
   To connect using RemoteXY mobile app by link http://remotexy.com/en/download/                   
     - for ANDROID 4.11.1 or later version;
     - for iOS 1.9.1 or later version;
    
   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.    
*/

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library
// #define REMOTEXY_MODE__ESP32CORE_BLE
// #include "BLEDevice.h"
// with my version of the library with Nimble support
#define REMOTEXY_MODE__ESP32CORE_NIMBLE
#include "NimBLEDevice.h"

#include <RemoteXY.h>

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "ESP32 Camper"


// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 327 bytes
  { 255,16,0,44,0,64,1,16,31,5,130,0,2,14,59,24,1,31,131,1,
  2,1,20,7,1,12,31,72,111,109,101,0,131,0,24,1,20,7,2,6,
  31,67,111,110,102,105,103,0,129,0,3,15,21,6,1,8,86,111,108,116,
  97,103,101,0,67,2,29,16,20,5,1,8,26,11,67,2,29,23,20,5,
  1,8,26,11,67,2,29,30,20,5,1,8,26,11,129,0,3,29,15,6,
  1,8,72,117,109,105,100,105,116,121,0,2,0,4,45,16,6,1,120,26,
  31,31,79,78,0,79,70,70,0,129,0,3,22,15,6,1,8,84,101,109,
  112,0,129,0,37,40,7,4,1,8,70,97,110,0,2,0,37,45,16,6,
  1,190,26,31,31,79,78,0,79,70,70,0,129,0,37,54,7,4,1,8,
  72,101,97,116,101,114,0,2,0,37,59,16,6,1,64,26,31,31,79,78,
  0,79,70,70,0,129,0,4,40,10,4,1,8,87,105,110,100,111,119,0,
  129,0,4,66,19,4,1,8,68,97,116,101,32,84,105,109,101,0,7,36,
  4,74,38,4,1,2,26,2,11,1,3,45,74,11,4,1,29,31,83,97,
  118,101,32,84,105,109,101,0,67,4,3,9,57,4,0,1,31,11,1,3,
  47,93,11,4,2,29,31,83,97,118,101,32,84,105,109,101,0,129,0,4,
  70,13,3,1,26,40,121,121,121,121,45,109,109,45,100,100,32,104,104,58,
  109,105,58,115,115,41,0 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t bWindow; // =1 if switch ON and =0 if OFF 
  uint8_t bFan; // =1 if switch ON and =0 if OFF 
  uint8_t bHeater; // =1 if switch ON and =0 if OFF 
  char txtTime[11];  // string UTF8 end zero  
  uint8_t btnSaveTime; // =1 if button pressed, else =0 
  uint8_t btnSaveConfig; // =1 if button pressed, else =0 

    // output variables
  char txtVoltage[11];  // string UTF8 end zero 
  char txtTemp[11];  // string UTF8 end zero 
  char txtHumidity[11];  // string UTF8 end zero 
  char lblMessage[11];  // string UTF8 end zero 

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

void BLE_APP_setup();

void BLE_APP_loop();
#endif
#endif