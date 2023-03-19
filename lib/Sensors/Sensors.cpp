#include "Arduino.h"
#ifdef SENSORS
#include "Sensors.h"

/*
IMPORTANT:
Voltage diveder need to read from ACS712 (output 0-5v, ESP32 reads 0-3.3v)
Ref. https://www.circuitschools.com/measure-ac-current-by-interfacing-acs712-sensor-with-esp32/
*/

Sensors::Sensors(int DHT11_pin, int ACS712_pin, int ACS712_amps)
{
  _DHT11_pin = DHT11_pin;
  _ACS712_pin = ACS712_pin;
  _ACS712_amps = ACS712_amps;
}

void Sensors::begin()
{
  if (_DHT11_pin != 0)
    dht11 = new SimpleDHT11(_DHT11_pin);

  if (_ACS712_pin != 0)
  {
    if (_ACS712_amps == 5)
      _mVperAmp = 185;
    if (_ACS712_amps == 20)
      _mVperAmp = 100;
    if (_ACS712_amps == 30)
      _mVperAmp = 66;
  }
};

void Sensors::read()
{

  if (millis() > lastCheck + maxSensorsPool)
  {
    if (_DHT11_pin != 0)
    {
      int err = SimpleDHTErrSuccess;
      if ((err = dht11->read2(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
      {
        Serial.print("Read DHT11 failed, err=");
        Serial.println(err);
      }
    }
    if (_ACS712_pin != 0)
    {
      _VoltageRead = getVPP();
      _VRMS = (_VoltageRead / 2.0) * 0.707; // root 2 is 0.707
      AmpsRMS = ((_VRMS * 1000) / _mVperAmp);
      Watt = (AmpsRMS*12);//amps x input Voltage
    }
    lastCheck = millis();
  }
}

float Sensors::getVPP()
{
  float result;
  int readValue;       // value read from the sensor
  int maxValue = 0;    // store max value here
  int minValue = 4096; // store min value here ESP32 ADC resolution

  uint32_t start_time = millis();
  while ((millis() - start_time) < 1000) // sample for 1 Sec
  {
    readValue = analogRead(_ACS712_pin);
    // see if you have a new maxValue
    if (readValue > maxValue)
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue)
    {
      /*record the minimum sensor value*/
      minValue = readValue;
    }
  }

  // Subtract min from max
  result = ((maxValue - minValue) * 3.3) / 4096.0; // ESP32 ADC resolution 4096

  return result;
}
#endif