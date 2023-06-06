#include "Arduino.h"
#ifdef SENSORS
#include "Sensors.h"
#include "esp_adc_cal.h"

#ifdef DHT11_pin
SimpleDHT11 *dht11;
#endif
#ifdef Servo_pin
Servo windowServo;
#endif

#ifdef EXT_DHT22_pin
SimpleDHT22 *dht22;

#endif

float _vref = 1100;

void initSensors()
{
#ifdef DHT11_pin
  dht11 = new SimpleDHT11(DHT11_pin);
#endif

#ifdef Voltage_pin
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  _vref = adc_chars.vref; // Obtain the device ADC reference voltage
#endif

#ifdef Servo_pin
  windowServo.attach(Servo_pin);
  // Init the window closed
  setWindow(false);
#endif

#ifdef Relay1_pin
  // Set pin to output
  pinMode(Relay1_pin, OUTPUT);
#endif

#ifdef Relay2_pin
  // Set pin to output
  pinMode(Relay2_pin, OUTPUT);
#endif

#ifdef EXT_DHT22_pin
  dht22 = new SimpleDHT22(EXT_DHT22_pin);
#endif
};

unsigned long lastCheck = 0;
unsigned long maxSensorsPool = 2500;

#ifdef Voltage_pin
float getVoltage()
{
  float result;
  float readValue; // value read from the sensor
  readValue = analogRead(Voltage_pin);

  // N.B. this formula assumes a 100k ohm based Voltage divider on the input voltage
  result = (readValue / 4095) // ADC Resolution (4096 = 0-4095)
           * VDiv_MaxVolt     // Max Input Voltage use during voltage divider calculation (Ex. 15)
           * (1100 / _vref)   // Device Calibration offset
           * (VDiv_Res_Calc   // Theorethical resistance calculated
              / VDiv_Res_Real // Real Installed resistance (Ex. 27k, or if you have not a 27k ... 20k + 5.1k + 2k in series )
              ) *
           VDiv_Calibration;

  // Notes on VDiv_Calibration
  // Calibration calculated by measurement with a multimiter
  //- 1: set VDiv_Calibration to 1
  //- 2: measure volt input with a multimiter (Ex. 13.22)
  //- 3: VDiv_Calibration = MULTIMETER_VOLTS/result
  //- 4: update platformio.ini and rebuild

  return result;
}
#endif

void readSensors()
{

  if (millis() > lastCheck + maxSensorsPool)
  {
#ifdef DHT11_pin
    int err = SimpleDHTErrSuccess;
    if ((err = dht11->read2(&last_Temperature, &last_Humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Read DHT11 failed, err=");
      Serial.println(err);
    }
#endif
#ifdef Voltage_pin
    last_Voltage = getVoltage();
#endif
#ifdef EXT_DHT22_pin

    if ((err = dht22->read2(&last_Ext_Temperature, &last_Ext_Humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Read EXT DHT22 failed, err=");
      Serial.println(err);
      last_Ext_Temperature = NAN;
      last_Ext_Humidity = NAN;
    }

#endif

    lastCheck = millis();
    last_Millis = lastCheck;
  }
}

#ifdef Servo_pin

const int closePos = 40;
const int openPos = 180;
const int servoDegreeDelay = 5;

int lastPos = -1;

void setWindow(bool isOpen)
{
  Serial.printf("isOpen: %d lastPos: %d LastWindow %u\n", isOpen, lastPos, last_WINDOW);
  if (lastPos == -1)
  {
    // Init reading the current position
    lastPos = windowServo.read();
  }

  if (last_WINDOW != isOpen)
  {
    int pos = lastPos;

    // Save value first than move to avoid multiple same command
    last_WINDOW = isOpen;

    if (isOpen)
    {
      // TODO: run in a different task
      //  Move to the Open Position
      for (pos = pos; pos <= openPos; pos++)
      {
        windowServo.write(pos);
        lastPos = pos;
        delay(servoDegreeDelay);
      }
    }
    else
    {
      // TODO: run in a different task
      //  Move to Close Position
      for (pos = pos; pos >= closePos; pos--)
      {
        windowServo.write(pos);
        lastPos = pos;
        delay(servoDegreeDelay);
      }
    }
  }
}
#endif

#ifdef Relay1_pin
void setFan(bool isOn)
{
  if (isOn)
  {
    digitalWrite(Relay1_pin, HIGH);
  }
  else
  {
    digitalWrite(Relay1_pin, LOW);
  }
  last_Relay1 = isOn;
};
#endif
#ifdef Relay2_pin
void setHeater(bool isOn)
{
  if (isOn)
  {
    digitalWrite(Relay2_pin, HIGH);
  }
  else
  {
    digitalWrite(Relay2_pin, LOW);
  }
  last_Relay2 = isOn;
};
#endif
#endif