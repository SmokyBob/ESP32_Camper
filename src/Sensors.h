#ifndef SENSORS_H
#define SENSORS_H

#include "Arduino.h"
#include "globals.h"
#ifdef DHT22_pin
#include "SimpleDHT.h"
#endif
#ifdef Servo_pin
#include "ESP32Servo.h"
#endif
#ifdef EXT_DHT22_pin
#include "SimpleDHT.h"
#endif
#if defined(EXT_SHT2_SDA) || defined(SHT2_SDA)
#include "SHT2x.h"
#if defined(CAMPER) || defined(HANDHELD)
#include "Wire.h"
#endif
#endif

#ifdef USE_MLX90614
#include "Adafruit_MLX90614.h"
#endif

void initSensors();
void readSensors();
#if defined(CAMPER) || defined(EXT_SENSORS)
void setWindow(bool isOpen);
void setFan(bool isOn);
void setHeater(bool isOn);

#endif
#endif