#if defined(CAMPER) || defined(HANDHELD)
#include "Arduino.h"
#include "LoraUtils.h"
#include "site.h"
#if defined(CAMPER)
#include "Sensors.h"
#endif
#include <ArduinoQueue.h>

#if defined(RADIO_SX1276)
SX1276 radio = nullptr;
float currentLimit = 120;
#endif

#if defined(RADIO_SX1262)
// #define VSPI FSPI to fix the pin mapping for the board
SPIClass spi(FSPI);
SPISettings spiSettings(100000, MSBFIRST, SPI_MODE0);
SX1262 radio = nullptr;
float currentLimit = 140; // Higher OCP limit for SX126x PA
#endif

int loraState = RADIOLIB_ERR_NONE;
volatile bool loraOperationDone = false;
volatile bool interruptEnabled = true;

bool transmitFlag = false;
// this function is called when a complete packet
// is received by the module or when transmission is complete
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!

void setLoraFlags(void)
{
  // we sent or received a packet, set the flag
  loraOperationDone = true;
}

void initLora()
{

  Serial.println("Starting LoRa");
#if defined(RADIO_SX1276)
  radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

  // SPI LoRa pins
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
#endif
#if defined(RADIO_SX1262)
  radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, spi, spiSettings);

  spi.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN); // SCK, MISO, MOSI, SS
#endif
  // When the power is turned on, a delay is required.
  delay(1500);
  int counter = 0;
  while (radio.begin(BAND) != RADIOLIB_ERR_NONE && counter < 10)
  {
    Serial.print(".");
    counter++;
    delay(500);
  }

  if (counter == 10)
  {
    Serial.println("Starting LoRa failed!");
  }
  else
  {
    Serial.println(F("success!"));
    radio.setOutputPower(LORA_POWER);
    radio.setBandwidth(LORA_BANDWIDTH);
    radio.setSpreadingFactor(LORA_SPREDING_FACTOR);
    radio.setCodingRate(LORA_CODING_RATE);
#if defined(RADIO_SX1262) == false
    radio.setCurrentLimit(currentLimit);
#endif

    // set the function that will be called
    // when new packet is received/transmitted
    radio.setPacketReceivedAction(setLoraFlags);
    radio.setPacketSentAction(setLoraFlags);

    Serial.print(F("Starting to transmit ... "));
    loraSend(String(DEVICE_NAME) + " online");

  }
}

ArduinoQueue<String> LoraSendQueue(20);

#if defined(HANDHELD)
unsigned long lastLoraReceive = 0;
unsigned long forceHello_dc = (LORA_DC / 2);
#endif

// LoRaData format:
// String examples (0 = Sensor Data):
// 0?enum.data.TEMP=36
// 0?enum.data.TEMP=36&enum.data.humidity=90&enum.data.VOLTS=13.22&enum.data.DATETIME=20230416113532
// 0?enum.data.relay1=0
// RealString
// 0?0=20&1=35&2=13.23&3=12065&4=20230416113532
// String examples (1 = Commands):
// 1?enum.data.relay1=1
// 1?enum.data.relay1=0
void handleLora()
{
#if defined(HANDHELD)
  // Check if the last received message was 1 sec more than the DC
  if (millis() > (lastLoraReceive + (forceHello_dc * 1000)))
  {
    // Force send a new "hello"
    loraSend(String(DEVICE_NAME) + " online");
    lastLoraReceive = millis();
    forceHello_dc = (LORA_DC / 2); // Force send half lora DC
  }
#endif
  // check if the flag is set
  if (loraOperationDone)
  {
    // reset flag
    loraOperationDone = false;

    if (transmitFlag)
    {
      last_SNR = radio.getSNR();
      last_RSSI = radio.getRSSI();

      //  clean up after transmission is finished
      //  this will ensure transmitter is disabled,
      //  RF switch is powered down etc.

      radio.finishTransmit();
      Serial.println("finshed transmit");

      // Reset the flag to process new messages / start receiving
      transmitFlag = false;
      Serial.println(F("transmitFlag = false"));
      // put module back to listen mode
      radio.startReceive();
      Serial.println(F("Start receive"));
    }
    else
    {
      // Check if there are elements to send in the queue
      if (LoraSendQueue.isEmpty())
      {
        // put module back to listen mode
        radio.startReceive();

        Serial.println(F("Force receive"));

        // if needed, 'listen' mode can be disabled by calling
        // any of the following methods:
        //
        // radio.standby()
        // radio.sleep()
        // radio.transmit();//I guess adio.startTransmit(); too?
        // radio.receive();
        // radio.readData();
        // radio.scanChannel();
      }
      else
      {
        // Get the message from the queue
        String message = LoraSendQueue.dequeue();
        // Send message
        radio.startTransmit(message);
        transmitFlag = true;

        Serial.print("sent Message:");
        Serial.println(message);
      }

      // if still in lisening mode read received message

      if (transmitFlag == false)
      {
        // you can read received data as an Arduino String
        String str;
        int state = radio.readData(str);
        // you can also read received data as byte array
        /*
          byte byteArr[8];
          int state = radio.readData(byteArr, 8);
        */

        Serial.print("receivedStr:");
        Serial.println(str);

        if (state == RADIOLIB_ERR_NONE)
        {

          // Ex. 0?0=20&1=35&2=13.23&3=12065&4=20230416113532
          int posCommand = str.indexOf('?');

          // Serial.printf("posCommand: %u\n", posCommand);

          if (posCommand > 0)
          {

            int type = str.substring(0, posCommand).toInt();
            // Remove type from string
            str = str.substring(posCommand + 1);
            // Serial.println(str);
            do
            {
              int dataEnum = str.substring(0, str.indexOf('=')).toInt();
              int idxValEnd = str.indexOf('&');
              String dataVal;
              if (idxValEnd > 0)
              {
                dataVal = str.substring(str.indexOf('=') + 1, idxValEnd);
              }
              else
              {
                dataVal = str.substring(str.indexOf('=') + 1);
              }

              if (type == DATA)
              {
                // Loop over the data array
                for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
                {
                  // Same id, update value
                  if (data[i].id == dataEnum)
                  {
                    data[i].value = dataVal;

                    // data with commands
                    if (strcmp(data[i].key, "B_WINDOW") == 0)
                    {
#ifdef Servo_pin
                      setWindow((dataVal == "1"));
#endif
#if defined(CAMPER)
                      // call EXT_SENSORS API to send the command
                      callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
                      // Force a lora send on next loop
                      lastLORASend = 0;
#endif
                    }
                    if (strcmp(data[i].key, "B_FAN") == 0)
                    {
#ifdef Relay1_pin
                      setFan((dataVal == "1"));
#endif
#if defined(CAMPER)
                      // call EXT_SENSORS API to send the command
                      callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
                      // Force a lora send on next loop
                      lastLORASend = 0;
#endif
                    }
                    if (strcmp(data[i].key, "B_HEATER") == 0)
                    {
#ifdef Relay2_pin
                      setHeater((dataVal == "1"));
#endif
#if defined(CAMPER)
                      // call EXT_SENSORS API to send the command
                      callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
                      // Force a lora send on next loop
                      lastLORASend = 0;
#endif
                    }

                    if (strcmp(data[i].key, "DATETIME") == 0)
                    {
                      setDateTime(dataVal);
                    }

                    break; // found, exit loop
                  }
                }
              }

              // type == CONFIGS not used in lora message but only in UI config

              // Remove the read data from the message
              if (idxValEnd > 0)
              {
                str = str.substring(idxValEnd + 1);
              }
              else
              {
                str = "";
              }
              // Serial.println(str);
            } while (str.length() > 0);

          }
          last_SNR = radio.getSNR();
          last_RSSI = radio.getRSSI();
        }
      }
    }
  }
};

void loraSend(String message)
{
#if defined(CAMPER)
  // Enqueue only if handheld is listening
  if (last_handheld_hello_millis > 0)
  {
#endif
    // Add string to Queue
    LoraSendQueue.enqueue(message);
    // Force lora handle in next loop
    loraOperationDone = true;
    Serial.println("lora message enqueued");
#if defined(CAMPER)
  }
#endif
};
#endif