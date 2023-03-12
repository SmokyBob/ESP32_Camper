#include "Arduino.h"
#include "Sensors.h"

Sensors::Sensors(int DHT11_pin)
{
  _DHT11_pin = DHT11_pin;
}

void Sensors::begin()
{
  dht11 = new SimpleDHT11(_DHT11_pin);
};

void Sensors::read()
{

  if (millis() > lastTempCheck + maxDHT11pool)
  {
    int err = SimpleDHTErrSuccess;
    if ((err = dht11->read2(&_temperature, &_humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Read DHT11 failed, err=");
      Serial.println(err);
      // delay(2000);
      // return;
    }
    lastTempCheck = millis();
  }
}

float Sensors::getTemperature()
{
  return _temperature;
};
float Sensors::getHumidity()
{
  return _humidity;
};
