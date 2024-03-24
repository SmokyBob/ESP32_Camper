
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
#if defined(EXT_SHT2_SDA) || defined(USE_SHT2)
SHT2x tempSensor;
#endif
#if defined(EXT_SHT2_SDA) || defined(USE_SHT2) || defined(USE_MLX90614)
TwoWire I2Cone = TwoWire(1);
#endif
#if defined(USE_MLX90614)
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif

float _vref = 1100;

void initSensors()
{
#ifdef DHT22_pin
  int_dht22 = new SimpleDHT22(DHT22_pin);
#endif

#if defined(Voltage_pin) || defined(VDiv_Batt_pin)
#if !defined(ARDUINO_heltec_wifi_lora_32_V3)
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  _vref = adc_chars.vref; // Obtain the device ADC reference voltage
#endif
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
  // Init heater off
  setHeater(false);
#endif

#ifdef Relay1_pin
  // Set pin to output
  pinMode(Relay1_pin, OUTPUT);
  // Init fan off
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
#if defined(USE_SHT2) || defined(USE_MLX90614)
  I2Cone.begin(SHT2_SDA, SHT2_SCL, 50000); // this frequency works for SHT2 and MLX90614 sensors
#endif
#ifdef USE_SHT2
  bool res = tempSensor.begin(&I2Cone);
  Serial.print("SHT2 init result:");
  Serial.println(res);
  tempSensor.reset();
#endif
#ifdef USE_MLX90614
  mlx.begin(0x5A, &I2Cone);
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
  voltCalibration = getConfigVal("VOLT_ACTUAL").toFloat();
#elif defined(VDiv_Batt_pin)

  voltPin = VDiv_Batt_pin;
  voltCalibration = VDiv_Calibration;
#endif

  readValue = analogRead(voltPin);

  // Serial.printf("Voltage read: %.2f\n", readValue);

  result = (readValue / 4095)  // ADC Resolution (4096 = 0-4095)
           * VDiv_MaxVolt      // Max Input Voltage use during voltage divider calculation (Ex. 15)
           * (1100 / _vref)    // Device Calibration offset
           * ((VDiv_Res_plus + // R1 or the resistence connected to positive (real value)
               VDiv_Res_gnd) / // R2 or the resistence connectect to GND (real value)
              VDiv_Res_gnd) *
           voltCalibration; // VDiv_Calibration;

  // Serial.printf("Voltage: %.2f _vref: %.2f VDiv_Calibration:%.4f\n", result, _vref, voltCalibration);

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

      // Serial.print("  volt[");
      // Serial.print(i);
      // Serial.print("] : ");
      // Serial.println(_voltArray[i]);
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
  String currVal = "";
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
    float last_Ext_Temperature = NAN;
    float last_Ext_Humidity = NAN;
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
    setDataVal("EXT_TEMP", String(last_Ext_Temperature));
    setDataVal("EXT_HUM", String(last_Ext_Humidity));
#endif

#if defined(EXT_SHT2_SDA) || defined(USE_SHT2)
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
        currVal = getDataVal("EXT_TEMP");
        if (currVal == "")
        {
          setDataVal("EXT_TEMP", String(tmpTemp));
        }
        else
        {
          if (tmpTemp >= -40)
          {

            // allow only change of 40 degs between readings otherwise treat it as an error
            if ((tmpTemp - currVal.toFloat()) <= 40)
            {
              setDataVal("EXT_TEMP", String(tmpTemp));
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
      currVal = getDataVal("EXT_HUM");
      if (!isnan(tmphum))
      {
        setDataVal("EXT_HUM", String(tmphum));
      }

#else
      if (!isnan(tmpTemp))
      {
#if defined(CAMPER)
        currVal = getDataVal("TEMP");
        if (currVal == "")
        {
          setDataVal("TEMP", String(tmpTemp));
        }
        else
        {
          // allow only change of 40 degs between readings otherwise treat it as an error
          if ((tmpTemp - currVal.toFloat()) <= 40)
          {
            setDataVal("TEMP", String(tmpTemp));
          }
          else
          {
            Serial.printf("                    Read temperature %.2f \n", tmpTemp);
          };
        }
        Serial.printf("temperature %.2f \n", last_Temperature);

#elif defined(HANDHELD)
        currVal = getDataVal("AMB_TEMP");
        if (currVal == "")
        {
          setDataVal("AMB_TEMP", String(tmpTemp));
        }
        else
        {
          // allow only change of 40 degs between readings otherwise treat it as an error
          if ((tmpTemp - currVal.toFloat()) <= 40)
          {
            setDataVal("AMB_TEMP", String(tmpTemp));
          }
          else
          {
            Serial.printf("                    Read temperature %.2f \n", tmpTemp);
          };
        }
#endif
      }
#if defined(CAMPER)
      currVal = getDataVal("HUM");
      if (!isnan(tmphum))
      {
        setDataVal("HUM", String(tmphum));
      }

      Serial.printf("hum %.2f \n", tmphum);
#elif defined(HANDHELD)
      currVal = getDataVal("AMB_HUM");
      if (!isnan(tmphum))
      {
        setDataVal("AMB_HUM", String(tmphum));
      }

      Serial.printf("hum %.2f \n", tmphum);
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
    setDataVal("VOLTS", String(getVoltage()));
#endif
#if defined(VDiv_Batt_pin)
    setDataVal("HAND_VOLTS", String(getVoltage()));
#endif

#if defined(USE_MLX90614)
    float tmpObjTemp = mlx.readObjectTempC();
    if (!isnan(tmpObjTemp))
    {
      setDataVal("IR_TEMP", String(tmpObjTemp));
    }
#if defined(USE_SHT2) == false
    float tmpTemp = mlx.readAmbientTempC();
    if (!isnan(tmpTemp))
    {
      setDataVal("AMB_TEMP", String(tmpTemp));
    }
#endif
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
  int closePos = getConfigVal("SERVO_CL_POS").toInt();
  int openPos = getConfigVal("SERVO_OP_POS").toInt();

  bool last_WINDOW = (getDataVal("B_WINDOW") == "1");

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

    if (isOpen)
    {
      // Save value first than move to avoid multiple same command
      setDataVal("B_WINDOW", "1");
      windowServo.write(openPos);
      lastPos = openPos;
    }
    else
    {
      // Save value first than move to avoid multiple same command
      setDataVal("B_WINDOW", "0");
      windowServo.write(closePos);
      lastPos = closePos;
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
    setDataVal("B_FAN", "1");
    digitalWrite(Relay1_pin, HIGH);
  }
  else
  {
    setDataVal("B_FAN", "0");
    digitalWrite(Relay1_pin, LOW);
  }
#endif
};

void setHeater(bool isOn)
{
#ifdef Relay2_pin
  if (isOn)
  {
    setDataVal("B_HEATER", "1");
    if (!heaterWithFan)
    {
      // Force fan ON if the heater has no internal fan
      setFan(true);
    }
    // Turn Heater Off
    digitalWrite(Relay2_pin, HIGH);
  }
  else
  {
    setDataVal("B_HEATER", "0");
    // Turn Heater Off, leave fan on
    digitalWrite(Relay2_pin, LOW);
  }
#endif
};
#endif