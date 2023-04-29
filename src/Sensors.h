#ifndef SENSORS_H
#define SENSORS_H
#include "Arduino.h"
#include "globals.h"
#ifdef SENSORS
#include "SimpleDHT.h"

void initSensors();
void readSensors();

#endif
#endif