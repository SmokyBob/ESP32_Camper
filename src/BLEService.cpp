#ifdef BLE_APP
#include "BLEService.h"

BLEServer *pServer = NULL;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "41cc63d6-8918-4f8d-ab01-e95e4155ee41"

struct myCharacteristics
{
  String name;
  String uuid;
  uint16_t properties;
  BLECharacteristic *refChar;
};

myCharacteristics charArray[8]{
    {"volts", "5b4c2c35-8a17-4d41-aec2-04a7dc1eaf91", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"ext_temperature", "226115b6-f631-4f82-b58d-b84487b55a64", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"ext_humidity", "b95cdb8a-7ee4-48c6-a818-fd11e60881f4", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"datetime", "2cdc00e8-907c-4f63-a284-2be098f8ea52", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"window", "4efa5b56-0426-42d7-857e-3ae3370b4a1d", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"relay1", "e8db3027-e095-435d-929c-f471669209c3", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"relay2", "4d15f090-6175-4e3c-b076-6ae0f69b7117", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"automation", "ea7614e2-7eb9-4e1c-8ac4-5e64c3994264", NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
};

bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
  void onWrite(NimBLECharacteristic *pCharacteristic)
  {
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onWrite(), value: ");
    std::string value = pCharacteristic->getValue();
    Serial.println(value.c_str());
    String dataVal = value.c_str();
    for (size_t i = 0; i < (sizeof(charArray) / sizeof(myCharacteristics)); i++)
    {
      if (charArray[i].uuid.equals(pCharacteristic->getUUID().toString().c_str()))
      {
        if (charArray[i].name == "volts")
        {
          last_Voltage = dataVal.toFloat();
        }

        if (charArray[i].name == "ext_temperature")
        {
          last_Temperature = dataVal.toFloat();
        }

        if (charArray[i].name == "ext_humidity")
        {
          last_Humidity = dataVal.toFloat();
        }

        if (charArray[i].name == "datetime")
        {
          last_DateTime = dataVal;
          setTime(last_DateTime);
        }

        if (charArray[i].name == "window")
        {
#if defined(CAMPER)
          callEXT_SENSORSAPI("api/1", String(WINDOW) + "=" + dataVal);
#else
          // TODO: manage handheld, send lora
#endif
          last_WINDOW = (dataVal.toInt() == 1);
        }

        if (charArray[i].name == "relay1")
        {

#if defined(CAMPER)
          callEXT_SENSORSAPI("api/1", String(RELAY1) + "=" + dataVal);
#else
          // TODO: manage handheld, send lora
#endif
          last_Relay1 = (dataVal.toInt() == 1);
        }

        if (charArray[i].name == "relay2")
        {
#if defined(CAMPER)
          callEXT_SENSORSAPI("api/1", String(RELAY2) + "=" + dataVal);
#else
          // TODO: manage handheld, send lora
#endif
          last_Relay2 = (dataVal.toInt() == 1);
        }

        if (charArray[i].name == "automation")
        {

#if defined(CAMPER)
          settings[8].value = dataVal.toFloat();
          callEXT_SENSORSAPI("api/2", String(CONFIG_ENABLE_AUTOMATION) + "=" + dataVal);
          savePreferences();
#else
          // TODO: manage handheld, send lora
#endif
        }
        break;
      }
    }
  };
};

/** Define callback instances globally to use for multiple Charateristics \ Descriptors */
static CharacteristicCallbacks chrCallbacks;

void initBLEService()
{
  String BLESERVER = "ESP BLE";

  // Create the BLE Device
  BLEDevice::init(BLESERVER.c_str());

  // TODO: check for a way to request pin to connect

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  for (size_t i = 0; i < (sizeof(charArray) / sizeof(myCharacteristics)); i++)
  {
    charArray[i].refChar = pService->createCharacteristic(
        NimBLEUUID(charArray[i].uuid.c_str()),
        charArray[i].properties);

    if ((charArray[i].properties & BLE_GATT_CHR_PROP_WRITE) != 0)
    {
      Serial.println("   Char id:" + charArray[i].name);
      charArray[i].refChar->setCallbacks(&chrCallbacks);
    }
  }

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.print("Service UID: ");
  Serial.println(SERVICE_UUID);
  Serial.println("Waiting a BLE client connection to notify...");
}

#ifdef BLE_APP
unsigned long lastBLENotify = 0;
#endif

void handleBLE()
{
  // notify changed value
  if (deviceConnected)
  {
    if ((millis() - lastBLENotify) > 1000)
    {
      for (size_t i = 0; i < (sizeof(charArray) / sizeof(myCharacteristics)); i++)
      {

        if (charArray[i].name == "volts")
        {
          charArray[i].refChar->setValue(last_Voltage);
        }

        if (charArray[i].name == "ext_temperature")
        {
          if (!isnan(last_Ext_Temperature))
            charArray[i].refChar->setValue(last_Ext_Temperature);
        }

        if (charArray[i].name == "ext_humidity")
        {
          if (!isnan(last_Ext_Humidity))
            charArray[i].refChar->setValue(last_Ext_Humidity);
        }

        if (charArray[i].name == "datetime")
        {
          charArray[i].refChar->setValue(last_DateTime);
        }

        if (charArray[i].name == "window")
        {
          charArray[i].refChar->setValue(String(last_WINDOW));
        }

        if (charArray[i].name == "relay1")
        {
          charArray[i].refChar->setValue(String(last_Relay1));
        }

        if (charArray[i].name == "relay2")
        {
          charArray[i].refChar->setValue(String(last_Relay2));
        }

        if (charArray[i].name == "automation")
        {
#if defined(CAMPER)
          charArray[i].refChar->setValue(String((int)settings[8].value));
#else
// TODO: send lora to camper
#endif
        }

        charArray[i].refChar->notify();
      }

      lastBLENotify = millis();
    }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
}
#endif