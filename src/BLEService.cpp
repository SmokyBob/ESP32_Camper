#ifdef BLE_APP
#include "BLEService.h"

BLEServer *pServer = NULL;

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
    // TODO: from nRF seems like write is not triggered
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onWrite(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
    // TODO: from the characteristic UUID call the correct action with the received value
  };
};

/** Define callback instances globally to use for multiple Charateristics \ Descriptors */
static CharacteristicCallbacks chrCallbacks;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "a6a7aaee-07da-4005-a5dc-64ee2bb30826"

// Temperature
#define TEMPERATURE_CHAR_UUID "94ff65cb-29f9-498c-85a7-5b52b0863155"
#define TEMPERATURE_DESC_UUID "edd8bcc4-1b2d-4a1f-b748-989e582a4728"
BLECharacteristic TEMPERATURE_Characteristic(TEMPERATURE_CHAR_UUID,
                                             NIMBLE_PROPERTY::NOTIFY |
                                                 NIMBLE_PROPERTY::READ);
BLEDescriptor *TEMPERATURE_Descriptor;

// Humidity
#define HUMIDITY_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define HUMIDITY_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic HUMIDITY_Characteristic(HUMIDITY_CHAR_UUID,
                                          NIMBLE_PROPERTY::NOTIFY |
                                              NIMBLE_PROPERTY::READ);

BLEDescriptor *HUMIDITY_Descriptor;

// TODO: change UIDS
// VOLTS
#define VOLTS_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define VOLTS_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic VOLTS_Characteristic(VOLTS_CHAR_UUID,
                                       NIMBLE_PROPERTY::NOTIFY |
                                           NIMBLE_PROPERTY::READ);

BLEDescriptor *VOLTS_Descriptor;

// WINDOW
#define WINDOW_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define WINDOW_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic WINDOW_Characteristic(WINDOW_CHAR_UUID,
                                        NIMBLE_PROPERTY::NOTIFY |
                                            NIMBLE_PROPERTY::READ |
                                            NIMBLE_PROPERTY::WRITE);

BLEDescriptor *WINDOW_Descriptor;

// FAN
#define FAN_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define FAN_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic FAN_Characteristic(FAN_CHAR_UUID,
                                     NIMBLE_PROPERTY::NOTIFY |
                                         NIMBLE_PROPERTY::READ |
                                         NIMBLE_PROPERTY::WRITE);

BLEDescriptor *FAN_Descriptor;

// HEATER
#define HEATER_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define HEATER_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic HEATER_Characteristic(HEATER_CHAR_UUID,
                                        NIMBLE_PROPERTY::NOTIFY |
                                            NIMBLE_PROPERTY::READ |
                                            NIMBLE_PROPERTY::WRITE);

BLEDescriptor *HEATER_Descriptor;

// EXT_TEMPERATURE
#define EXT_TEMPERATURE_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define EXT_TEMPERATURE_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic EXT_TEMPERATURE_Characteristic(EXT_TEMPERATURE_CHAR_UUID,
                                                 NIMBLE_PROPERTY::NOTIFY |
                                                     NIMBLE_PROPERTY::READ);

BLEDescriptor *EXT_TEMPERATURE_Descriptor;

// EXT_HUMIDITY
#define EXT_HUMIDITY_CHAR_UUID "d72a475e-2a87-4a37-aba1-dc8d883ba7ee"
#define EXT_HUMIDITY_DESC_UUID "0db248b6-dc38-4766-b99f-e7f38f97f32a"
BLECharacteristic EXT_HUMIDITY_Characteristic(EXT_HUMIDITY_CHAR_UUID,
                                              NIMBLE_PROPERTY::NOTIFY |
                                                  NIMBLE_PROPERTY::READ);

BLEDescriptor *EXT_HUMIDITY_Descriptor;

void initBLEService()
{
  String BLESERVER = String(DEVICE_NAME) + " BLE";

  // Create the BLE Device
  BLEDevice::init(BLESERVER.c_str());

  // TODO: check a way to request pin to connect, otherwise client considered not connected

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // TODO: add Characteristics
  //  temperature Characteristic
  pService->addCharacteristic(&TEMPERATURE_Characteristic);
  // // Create a BLE Descriptor
  // TEMPERATURE_Descriptor = TEMPERATURE_Characteristic.createDescriptor(TEMPERATURE_DESC_UUID,
  //                                                                      NIMBLE_PROPERTY::NOTIFY |
  //                                                                          NIMBLE_PROPERTY::READ);
  // TEMPERATURE_Descriptor->setValue("Temperature");

  // TODO: other characteristics

  //  temperature Characteristic
  pService->addCharacteristic(&WINDOW_Characteristic);
  // // Create a BLE Descriptor
  // WINDOW_Descriptor = WINDOW_Characteristic.createDescriptor(WINDOW_DESC_UUID,
  //                                                            NIMBLE_PROPERTY::NOTIFY |
  //                                                                NIMBLE_PROPERTY::READ |
  //                                                                NIMBLE_PROPERTY::WRITE |
  //                                                                NIMBLE_PROPERTY::READ_ENC |
  //                                                                NIMBLE_PROPERTY::WRITE_ENC |
  //                                                                NIMBLE_PROPERTY::WRITE_ENC // only allow writing if paired / encrypted
  // );

  // WINDOW_Descriptor->setValue("Window");

  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  pAdvertising->start();

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
      TEMPERATURE_Characteristic.setValue(last_Temperature);
      TEMPERATURE_Characteristic.notify();

      // TODO: notify other values
      WINDOW_Characteristic.setValue(String(last_WINDOW));
      WINDOW_Characteristic.notify();

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