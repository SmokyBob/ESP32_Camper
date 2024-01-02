
#include "Arduino.h"
#include "Sensors.h"
#include "esp_adc_cal.h"

#ifdef DHT22_pin
SimpleDHT22 *int_dht22;
#endif
#ifdef Servo_pin
Servo windowServo;
#endif

#ifdef EXT_DHT22_pin
SimpleDHT22 *ext_dht22;

#endif
#if defined(EXT_SHT2_SDA) || defined(SHT2_SDA)
SHT2x tempSensor;
#if defined(CAMPER) || defined(HANDHELD)
TwoWire I2Cone = TwoWire(1);
#endif
#endif

float _vref = 1100;

void initSensors()
{
#ifdef DHT22_pin
  int_dht22 = new SimpleDHT22(DHT22_pin);
#endif

#if defined(Voltage_pin) || defined(VDiv_Batt_pin)
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  _vref = adc_chars.vref; // Obtain the device ADC reference voltage
#endif

#ifdef Servo_pin
  windowServo.setPeriodHertz(50);
  windowServo.attach(Servo_pin); // attach servo
  // Init the window closed
  setWindow(false);
#endif

#ifdef Relay2_pin
  // Set pin to output
  pinMode(Relay2_pin, OUTPUT);
  //Init heater off
  setHeater(false);
#endif

#ifdef Relay1_pin
  // Set pin to output
  pinMode(Relay1_pin, OUTPUT);
  //Init fan off
  setFan(false);
#endif

#ifdef EXT_DHT22_pin
  ext_dht22 = new SimpleDHT22(EXT_DHT22_pin);
  // ext_dht22->setPinInputMode(INPUT_PULLDOWN);
#endif
#ifdef EXT_SHT2_SDA
  bool res = tempSensor.begin(EXT_SHT2_SDA, EXT_SHT2_SCL);
  Serial.print("EXT_SHT2 init result:");
  Serial.println(res);
#endif
#ifdef SHT2_SDA
  I2Cone.begin(SHT2_SDA, SHT2_SCL);
  bool res = tempSensor.begin(&I2Cone);
  Serial.print("SHT2 init result:");
  Serial.println(res);
  tempSensor.reset();
#endif
};

unsigned long lastCheck = 0;
unsigned long maxSensorsPool = 2500;

#if defined(Voltage_pin) || defined(VDiv_Batt_pin)
float _voltArray[5];
bool voltInitComplete = false;
byte voltArrayindex = 0;

void calculateVoltage()
{
  float result;
  float readValue; // value read from the sensor
  uint8_t voltPin;
  float voltCalibration;

#if defined(Voltage_pin)
  voltPin = Voltage_pin;
  voltCalibration = settings[4].value;
#elif defined(VDiv_Batt_pin)

  voltPin = VDiv_Batt_pin;
  voltCalibration = VDiv_Calibration;
#endif

  readValue = analogRead(voltPin);

  result = (readValue / 4095)  // ADC Resolution (4096 = 0-4095)
           * VDiv_MaxVolt      // Max Input Voltage use during voltage divider calculation (Ex. 15)
           * (1100 / _vref)    // Device Calibration offset
           * ((VDiv_Res_plus + // R1 or the resistence connected to positive (real value)
               VDiv_Res_gnd) / // R2 or the resistence connectect to GND (real value)
              VDiv_Res_gnd) *
           voltCalibration; // VDiv_Calibration;

  // Serial.printf("Voltage: %.2f VDiv_Calibration:%.4f\n", result, settings[4].value);

  // Notes on VDiv_Calibration
  // Calibration calculated by measurement with a multimiter
  //- 1: set VDiv_Calibration to 1
  //- 2: measure volt input with a multimiter (Ex. 13.22)
  //- 3: VDiv_Calibration = MULTIMETER_VOLTS/result
  //- 4: update platformio.ini and rebuild

  _voltArray[voltArrayindex] = result;
  voltArrayindex = voltArrayindex + 1;
  // Serial.printf("Voltage idx: %u\n", voltArrayindex);
  if (voltArrayindex == 5)
  {
    voltArrayindex = 0;
    if (voltInitComplete == false)
    {
      voltInitComplete = true;
      Serial.printf("Voltage Init complete\n\n");
    }
  }
}

float getVoltage()
{
  if (voltInitComplete)
  {

    double sum = 0.00; // sum will be larger than an item, double for safety.
    for (int i = 0; i < 5; i++)
    {
      sum += _voltArray[i];

      Serial.print("  volt[");
      Serial.print(i);
      Serial.print("] : ");
      Serial.println(_voltArray[i]);
    }

    return ((float)sum) / 5; // average will be fractional, so float may be appropriate.
  }
  else
  {
    return _voltArray[voltArrayindex];
  }
}
#endif

unsigned long voltTick = 0;

void readSensors()
{
#if defined(Voltage_pin) || defined(VDiv_Batt_pin)
  // Calculate the voltage every second
  if ((millis() - voltTick) > 1000)
  {
    calculateVoltage();
    voltTick = millis();
  }
#endif

  if (millis() > lastCheck + maxSensorsPool)
  {
#if defined(DHT22_pin) || defined(EXT_DHT22_pin)
    int err = SimpleDHTErrSuccess;
#ifdef DHT22_pin

    if ((err = int_dht22->read2(&last_Temperature, &last_Humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Read Internal DHT22 failed, err=");
      Serial.println(err);
    }
    // Serial.printf("temp %.2f \n", last_Temperature);
    // Serial.printf("hum %.2f \n", last_Humidity);
#endif
#ifdef EXT_DHT22_pin

    if ((err = ext_dht22->read2(&last_Ext_Temperature, &last_Ext_Humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Read EXT DHT22 failed, err=");
      Serial.println(SimpleDHTErrCode(err));
    }
    // Serial.printf("temp %.2f \n", last_Ext_Temperature);
    // Serial.printf("hum %.2f \n", last_Ext_Humidity);
#endif
#endif

#if defined(EXT_SHT2_SDA) || defined(SHT2_SDA)
    uint8_t stat = tempSensor.getStatus();
    Serial.print("SHT2 status: ");
    Serial.println(stat, HEX);
    bool readOK = tempSensor.read();
    float tmpTemp = tempSensor.getTemperature();
    float tmphum = tempSensor.getHumidity();
    if (readOK)
    {
#ifdef EXT_SHT2_SDA
      if (!isnan(tmpTemp))
      {
        if (isnan(last_Ext_Temperature))
        {
          last_Ext_Temperature = tmpTemp;
        }
        else
        {
          if (tmpTemp >= -40)
          {

            // allow only change of 40 degs between readings otherwise treat it as an error
            if ((tmpTemp - last_Ext_Temperature) <= 40)
            {
              last_Ext_Temperature = tmpTemp;
            }
            else
            {
              Serial.printf("                    Read temperature %.2f \n", tmpTemp);
            }
          }
          else
          {
            Serial.printf("                    Read temperature %.2f \n", tmpTemp);
          }
        }
      }
      if (!isnan(tmphum))
      {
        last_Ext_Humidity = tmphum;
      }

      Serial.printf("EXT temperature %.2f \n", last_Ext_Temperature);
      Serial.printf("EXT hum %.2f \n", last_Ext_Humidity);
#else
      if (!isnan(tmpTemp))
      {
#if defined(CAMPER)
        if (isnan(last_Temperature))
        {
          last_Temperature = tmpTemp;
        }
        else
        {
          // allow only change of 40 degs between readings otherwise treat it as an error
          if ((tmpTemp - last_Temperature) <= 40)
          {
            last_Temperature = tmpTemp;
          }
          else
          {
            Serial.printf("                    Read temperature %.2f \n", tmpTemp);
          };
        }
        Serial.printf("temperature %.2f \n", last_Temperature);

#elif defined(HANDHELD)
        if (isnan(hand_Temperature))
        {
          hand_Temperature = tmpTemp;
        }
        else
        {
          // allow only change of 40 degs between readings otherwise treat it as an error
          if ((tmpTemp - hand_Temperature) <= 40)
          {
            hand_Temperature = tmpTemp;
          }
          else
          {
            Serial.printf("                    Read temperature %.2f \n", tmpTemp);
          };
        }
#endif
      }
#if defined(CAMPER)
      if (!isnan(tmphum))
      {
        last_Humidity = tmphum;
      }

      Serial.printf("hum %.2f \n", last_Humidity);
#elif defined(HANDHELD)
      if (!isnan(tmphum))
      {
        hand_Humidity = tmphum;
      }

      Serial.printf("hum %.2f \n", hand_Humidity);
#endif

#endif
    }
    else
    {
      Serial.print("SHT2 error: ");
      Serial.println(tempSensor.getError(), HEX);
    }

#endif

#if defined(Voltage_pin)
    last_Voltage = getVoltage();
#endif
#if defined(VDiv_Batt_pin)
    batt_Voltage = getVoltage();
#endif

    lastCheck = millis();
    last_Millis = lastCheck;
  }
}
#if defined(CAMPER) || defined(EXT_SENSORS)
int servoDegreeDelay = 5;

int lastPos = -1;

void setWindow(bool isOpen)
{
#ifdef Servo_pin
  int closePos = (int)settings[0].value;
  int openPos = (int)settings[1].value;

  Serial.printf("isOpen: %d lastPos: %d LastWindow %u\n", isOpen, lastPos, last_WINDOW);
  Serial.printf("closePos: %d openPos: %d\n", closePos, openPos);
  if (lastPos < 0)
  {
    lastPos = closePos;
    windowServo.write(closePos);
  }

  if (last_WINDOW != isOpen)
  {
    int pos = lastPos;

    // Save value first than move to avoid multiple same command
    last_WINDOW = isOpen;

    if (isOpen)
    {
      windowServo.write(openPos);
      lastPos = openPos;
      // // TODO: run in a different task for BLE APP
      // //  Move to the Open Position
      // for (pos = pos; pos <= openPos; pos++)
      // {
      //   windowServo.write(pos);
      //   lastPos = pos;
      //   delay(servoDegreeDelay);
      // }
    }
    else
    {
      windowServo.write(closePos);
      lastPos = closePos;
      // // TODO: run in a different task for BLE APP
      // //  Move to Close Position
      // for (pos = pos; pos >= closePos; pos--)
      // {
      //   windowServo.write(pos);
      //   lastPos = pos;
      //   delay(servoDegreeDelay);
      // }
    }
  }

  Serial.printf("windowServo.read: %d\n", windowServo.read());

  // windowServo.detach(); // detach to avoid jitter
#endif
}

void setFan(bool isOn)
{
#ifdef Relay1_pin
  if (isOn)
  {
    digitalWrite(Relay1_pin, HIGH);
  }
  else
  {
    digitalWrite(Relay1_pin, LOW);
  }
  last_Relay1 = isOn;
#endif
};

void setHeater(bool isOn)
{
#ifdef Relay2_pin
  if (isOn)
  {
    // Force fan ON
    setFan(true);
    // Turn Heater Off
    digitalWrite(Relay2_pin, HIGH);
  }
  else
  {
    // Turn Heater Off, leave fan on
    digitalWrite(Relay2_pin, LOW);
  }
  last_Relay2 = isOn;
#endif
};
#endif