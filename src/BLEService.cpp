#ifdef BLE_APP
#include "BLEService.h"

BLEServer *pServer = NULL;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "41cc63d6-8918-4f8d-ab01-e95e4155ee41"

struct myCharacteristics
{
  char name[15];
  dataType arraySrc;
  uint16_t properties;
  BLECharacteristic *refChar;
};

myCharacteristics charArray[9]{
    {"VOLTS", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"EXT_TEMP", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"EXT_HUM", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ, nullptr},
    {"DATETIME", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"B_WINDOW", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"B_FAN", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"B_HEATER", DATA, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"B_AUTOMATION", CONFIGS, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
    {"B_VOLT_LIM_IGN", CONFIGS, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nullptr},
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
    bool found = false;
    for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
    {
      if (data[i].ble_uuid.equals(pCharacteristic->getUUID().toString().c_str()))
      {
        data[i].value = dataVal;
        found = true;

        // data with actions
        if (strcmp(data[i].key, "DATETIME") == 0)
        {
          setDateTime(dataVal);
#if defined(HANDHELD)
          // send lora command
          String LoRaMessage = String(DATA) + "?";
          LoRaMessage += String(data[i].id) + "=" + dataVal + "&";
          LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
          // Serial.println(LoRaMessage);
          loraSend(LoRaMessage);
#endif
        }
        if (strcmp(data[i].key, "B_WINDOW") == 0 || strcmp(data[i].key, "B_FAN") == 0 || strcmp(data[i].key, "B_HEATER") == 0)
        {
#if defined(CAMPER)
          callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
#endif
          // send lora command
          String LoRaMessage = String(DATA) + "?";
          LoRaMessage += String(data[i].id) + "=" + dataVal + "&";
          LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
          // Serial.println(LoRaMessage);
          loraSend(LoRaMessage);
        }
        break; // found, exit loop
      }
    }

    if (!found)
    {
      for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
      {
        if (config[i].ble_uuid.equals(pCharacteristic->getUUID().toString().c_str()))
        {
          config[i].value = dataVal;
          found = true;
          // configs with actions
          if (strcmp(config[i].key, "B_AUTOMATION") == 0)
          {
#if defined(CAMPER)
            callEXT_SENSORSAPI("api/2", String(config[i].id) + "=" + dataVal);
            savePreferences();
#endif
            // send lora command
            String LoRaMessage = String(CONFIGS) + "?";
            LoRaMessage += String(config[i].id) + "=" + dataVal + "&";
            LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
            // Serial.println(LoRaMessage);
            loraSend(LoRaMessage);
          }
          if (strcmp(config[i].key, "B_220POWER") == 0)
          {
            if (dataVal.toInt() == 1)
            {
              struct tm timeinfo;
              getLocalTime(&timeinfo);
              char buf[100];
              strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

              last_IgnoreLowVolt = String(buf);
            }
            else
            {
              last_IgnoreLowVolt = "";
            }

            Serial.print("            last_IgnoreLowVolt: ");
            Serial.println(last_IgnoreLowVolt);
#if defined(CAMPER)
            callEXT_SENSORSAPI("api/2", String(config[i].id) + "=" + dataVal);
#endif
            // send lora command
            String LoRaMessage = String(CONFIGS) + "?";
            LoRaMessage += String(config[i].id) + "=" + dataVal + "&";
            LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
            // Serial.println(LoRaMessage);
            loraSend(LoRaMessage);
          }

          break; // found, exit loop
        }
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
    String uuid;
    if (charArray[i].arraySrc == DATA)
    {
      uuid = getDataObj(charArray[i].name)->ble_uuid;
    }
    if (charArray[i].arraySrc == CONFIGS)
    {
      uuid = getConfigObj(charArray[i].name)->ble_uuid;
    }
    charArray[i].refChar = pService->createCharacteristic(
        NimBLEUUID(uuid.c_str()), charArray[i].properties);

    if ((charArray[i].properties & BLE_GATT_CHR_PROP_WRITE) != 0)
    {
      Serial.printf("   Char id: %s \n", charArray[i].name);
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
        String dataVal;
        if (charArray[i].arraySrc == DATA)
        {
          dataVal = getDataVal(charArray[i].name);
        }
        if (charArray[i].arraySrc == CONFIGS)
        {
          dataVal = getConfigVal(charArray[i].name);
        }

        // Characteristics with custom checks
        if (charArray[i].name == "B_220POWER")
        {
          dataVal = "0";
          if (last_IgnoreLowVolt != "")
          {
            dataVal = "1";
          }
        }

        charArray[i].refChar->setValue(dataVal);

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