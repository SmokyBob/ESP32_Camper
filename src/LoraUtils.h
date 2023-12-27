#ifndef LORA_H
#define LORA_H
#include "Arduino.h"
#include "globals.h"

#ifdef E220
#include "LoRa_E220.h"
#else
// Libraries for LoRa
#include <SPI.h>
#include <RadioLib.h>

// 433.0 for Asia/europe
// 866.0 for Europe
// 915.0 for North America
#define BAND 868.0

#define LORA_DC 20    // Wait Seconds to be safe on duty cycle LORA Usage
#define LORA_POWER 20 // dbi
//Config as long_moderate in meshtastic
#define LORA_BANDWIDTH 125
#define LORA_SPREDING_FACTOR 11
#define LORA_CODING_RATE 8

#endif

#if defined(RADIO_SX1276)
extern SX1276 radio;
#endif

#if defined(RADIO_SX1262)
extern SX1262 radio;
#endif

// save transmission state between loops
extern int loraState;

// flag to indicate that a packet was sent or received
extern volatile bool loraOperationDone;
// disable interrupt when it's not needed
extern volatile bool interruptEnabled;
// flag to indicate transmission or reception state
extern bool transmitFlag;

void initLora();

void loraReceive();
void loraSend(String message);
#endif