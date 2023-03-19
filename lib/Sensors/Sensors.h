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
  int _ACS712_pin;  // 0 = disabled
  int _ACS712_amps; // used for scaling, possible values 5,20,30

  int _mVperAmp = 0;
  double _VoltageRead = 0;
  double _VRMS = 0;

  unsigned long lastCheck = 0;
  unsigned long maxSensorsPool = 1500;
  float getVPP();

public:
  Sensors(int DHT11_pin, int ACS712_pin, int ACS712_amps);

  void begin();
  void read();

  float temperature = 0;
  float humidity = 0;

  int Watt = 0;
  double AmpsRMS = 0;
};
#endif