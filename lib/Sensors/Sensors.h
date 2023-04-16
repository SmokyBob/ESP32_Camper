#pragma once
#include "Arduino.h"
#ifdef SENSORS
#include "SimpleDHT.h"

class Sensors
{
private:
  /* data */
  int _DHT11_pin; // 0 = disabled
  SimpleDHT11 *dht11;

  int _voltagePin; // 0 = disabled
  int _vref;

  unsigned long lastCheck = 0;
  unsigned long maxSensorsPool = 1500;

  float getVoltage();

public:
  Sensors();
  Sensors(int currDHT11_pin, int currVoltage_pin);

  void begin();
  void read();

  float temperature = 0;
  float humidity = 0;

  float voltage = 0;
};
#endif