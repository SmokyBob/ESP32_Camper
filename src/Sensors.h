#ifndef SENSORS_H
#define SENSORS_H
#include "Arduino.h"
#include "globals.h"
#ifdef SENSORS
#ifdef DHT11_pin
#include "SimpleDHT.h"
#endif
#ifdef Servo_pin
#include "Servo.h"
#endif
#ifdef EXT_DHT22_pin
#include "SimpleDHT.h"
#endif

void initSensors();
void readSensors();

#ifdef Servo_pin
void setWindow(bool isOpen);
#endif

#endif
#endif