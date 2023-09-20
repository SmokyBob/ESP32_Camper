#ifndef SENSORS_H
#define SENSORS_H
#if defined(CAMPER) || defined(EXT_SENSORS)
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

void initSensors();
void readSensors();

void setWindow(bool isOpen);
void setFan(bool isOn);
void setHeater(bool isOn);

#endif
#endif