#ifndef SENSORS_H
#define SENSORS_H
#include "Arduino.h"
#include "globals.h"
#ifdef SENSORS
#include "SimpleDHT.h"
#include "Servo.h"

void initSensors();
void readSensors();

#ifdef Servo_pin
void setWindow(bool isOpen);
#endif

#endif
#endif