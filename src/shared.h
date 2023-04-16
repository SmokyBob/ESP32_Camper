#pragma once
#include "Arduino.h"
#include "enums.h"

#ifdef OLED
// Libraries for OLED Display
#include <Wire.h>
// #include <SPI.h> //maybe needed for U8g2lib
#include <U8g2lib.h>

// OLED pins for TTGO_LORA_V1
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

// Icons
const unsigned char epd_cell_tower[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x0c, 0x30, 0x24, 0x2c, 0x94, 0x29, 0x94, 0x29, 0xb4, 0x2d,
    0xa4, 0x21, 0xc8, 0x13, 0x40, 0x02, 0x40, 0x02, 0xe0, 0x03, 0x20, 0x06, 0x00, 0x00, 0x00, 0x00};
const unsigned char epd_hourglass[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0xf8, 0x1f, 0x30, 0x0c, 0x10, 0x08, 0x30, 0x0c, 0x20, 0x04, 0xc0, 0x03,
    0xc0, 0x03, 0x20, 0x04, 0x30, 0x0c, 0x10, 0x08, 0x30, 0x0c, 0xf8, 0x1f, 0x00, 0x00, 0x00, 0x00};
const unsigned char epd_timer[] PROGMEM = {
    0x00, 0x00, 0xc0, 0x03, 0x00, 0x00, 0xc0, 0x03, 0xe0, 0x1e, 0x10, 0x18, 0x88, 0x11, 0x88, 0x11,
    0x8c, 0x31, 0x08, 0x30, 0x08, 0x10, 0x18, 0x18, 0x30, 0x0c, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00};
const unsigned char epd_termostat[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x40, 0x02, 0x40, 0x03, 0x40, 0x02, 0x40, 0x02, 0xc0, 0x03,
    0xc0, 0x03, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xc0, 0x03, 0x00, 0x00, 0x00, 0x00};
const unsigned char epd_humidity[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xc0, 0x02, 0x60, 0x06, 0x10, 0x08, 0x18, 0x18, 0x08, 0x10,
    0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x10, 0x08, 0xe0, 0x07, 0x00, 0x00, 0x00, 0x00};
const unsigned char epd_bolt[] PROGMEM = {
    0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x80, 0x03, 0x80, 0x02, 0x40, 0x00, 0x20, 0x01, 0x30, 0x1e,
    0x10, 0x08, 0x78, 0x04, 0x40, 0x02, 0x40, 0x01, 0xc0, 0x01, 0xe0, 0x00, 0x60, 0x00, 0x00, 0x00};

#endif

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

class shared
{
private:
/* data */
#ifdef OLED
  // Init display object
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr;
#endif

#ifdef E220
// TODO:.... Radio lib SHOULD support lcXXXX chip... maybe it's just a matter on init?
#else
  SX1276 radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

  // save transmission state between loops
  int loraState = RADIOLIB_ERR_NONE;

  // flag to indicate that a packet was sent or received
  volatile bool loraOperationDone = false;
  // disable interrupt when it's not needed
  volatile bool interruptEnabled = true;
  // flag to indicate transmission or reception state
  bool transmitFlag = false;

  // TODO:capire come fa meshtastic a usarla
  void setupInterrupt(void (*callback)()) { radio.setDio0Action(callback); }

  // this function is called when a complete packet
  // is received by the module or when transmission is complete
  // IMPORTANT: this function MUST be 'void' type
  //            and MUST NOT have any arguments!
  static void setFlags(void);

#endif

public:
  shared(/* args */);
  ~shared();
  static shared &getInstance()
  {
    static shared instance;
    return instance;
  }

  String device_name = DEVICE_NAME;

#ifndef SENSORS
  // TODO: array of values with same index as enums.data ?
  float last_Temp;
  float last_Hum;
  float last_Voltage;
#endif
  bool last_WINDOW;
  bool last_Relay1;
  bool last_Relay2;
  u_long last_Millis;
  String last_DateTime;

  float last_SNR;
  float last_RSSI;

  void begin();
#ifdef OLED
  // TODO: usare menu
  //  if CAMPER shows sensor data, if HANDHELD shows RECEIVED data
  void pageSensors();

  // Wifi infos,
  void pageCongifs();
#endif

  void loraReceive();
  void loraSend(String message);
};
