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

// define the pins used by the LoRa transceiver module
#define RADIO_SCLK_PIN 5
#define RADIO_MISO_PIN 19
#define RADIO_MOSI_PIN 27
#define RADIO_CS_PIN 18
#define RADIO_DIO0_PIN 26
#define RADIO_RST_PIN 14
#define RADIO_DIO1_PIN 33
#define RADIO_BUSY_PIN 32

// 433.0 for Asia/europe
// 866.0 for Europe
// 915.0 for North America
#define BAND 868.0
// TODO: see meshtastic AirTime check... to be legally compliant
#define LORA_DC 10    // Wait Seconds to be safe on duty cycle LORA Usage
#define LORA_POWER 20 // dbi
#define LORA_BANDWIDTH 62.5
#define LORA_SPREDING_FACTOR 10

#endif

#ifdef E220
// TODO:.... Radio lib SHOULD support lcXXXX chip... maybe it's just a matter on init?
#else
extern SX1276 radio;

// save transmission state between loops
extern int loraState;

// flag to indicate that a packet was sent or received
extern volatile bool loraOperationDone;
// disable interrupt when it's not needed
extern volatile bool interruptEnabled;
// flag to indicate transmission or reception state
extern bool transmitFlag;

#endif

void initLora();

void loraReceive();
void loraSend(String message);
#endif