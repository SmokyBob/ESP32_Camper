#pragma once
#include "Arduino.h"
#ifdef SENSORS
#include "SimpleDHT.h"

class Sensors
{
private:
  /* data */
  int _DHT11_pin;
  SimpleDHT11 *dht11;

  float _temperature = 0;
  float _humidity = 0;

  unsigned long lastTempCheck = 0;
  unsigned long maxDHT11pool = 1500;
public:
  Sensors(int DHT11_pin);

  void begin();
  void read();

  float getTemperature();
  float getHumidity();
};
#endif