#ifndef OLED_UTIL_H
#define OLED_UTIL_H

#include "Arduino.h"
#include "globals.h"
#include "OneButton.h"

// Libraries for OLED Display
#include <Wire.h>
#include <U8g2lib.h>

// OLED pins for TTGO_LORA_V1
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

struct menu_entry_type
{
  const uint8_t *font;
  uint16_t icon;
  const char *name;
  void (*drawFunc)();
};

struct menu_state
{
  int16_t menu_start;     /* in pixel */
  int16_t frame_position; /* in pixel */
  uint8_t position;       /* position, array index */
};

extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2;

void initOled();

void turnOffOled();
void turnOnOled();

void drawPage();
#endif