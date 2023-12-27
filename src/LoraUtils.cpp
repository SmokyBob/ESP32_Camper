
#if defined(CAMPER) || defined(HANDHELD)
#include "Arduino.h"
#include "LoraUtils.h"
#include "site.h"
#if defined(CAMPER)
#include "Sensors.h"
#endif

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
  if (!interruptEnabled)
  {
    return;
  }
  // we sent or received a packet, set the flag
  loraOperationDone = true;
  //  if we were transmitting, cleanup and delay are needed
  if (transmitFlag)
  {

    //  clean up after transmission is finished
    //  this will ensure transmitter is disabled,
    //  RF switch is powered down etc.
    radio.finishTransmit();

    // Start recieve mode back up
    radio.startReceive();
    transmitFlag = false;
    loraOperationDone = false;
    interruptEnabled = true;
  }
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
    radio.setCurrentLimit(currentLimit);

    // set the function that will be called
    // when new packet is received/transmitted
    radio.setPacketReceivedAction(setLoraFlags);
    radio.setPacketSentAction(setLoraFlags);
// different initial state for CAMPER and HANDHELD
#ifdef CAMPER
    Serial.print(F("Starting to transmit ... "));
    loraState = radio.startTransmit(String(DEVICE_NAME) + " online");
    transmitFlag = true;
#endif
#ifdef HANDHELD
    loraState = radio.startReceive();
    transmitFlag = false;
#endif
  }
}

// LoRaData format:
// String examples (0 = Sensor Data):
//  0?enum.data.TEMP=36
//  0?enum.data.TEMP=36&enum.data.humidity=90&enum.data.VOLTS=13.22&enum.data.DATETIME=20230416113532
//  0?enum.data.relay1=0
// RealString
//  0?0=20&1=35&2=13.23&3=12065&4=20230416113532
// String examples (1 = Commands):
//  1?enum.data.relay1=1
//  1?enum.data.relay1=0
void loraReceive()
{
  // check if the flag is set
  if (loraOperationDone)
  {
    // disable the interrupt service routine while
    // processing the data
    interruptEnabled = false;

    // reset flag
    loraOperationDone = false;

    // you can read received data as an Arduino String
    String str;
    int state = radio.readData(str); // TODO: struct instead of a human readable string? should be a smaller payload
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
            switch (dataEnum)
            {
#if not(defined(CAMPER))
            case TEMPERATURE:
              last_Temperature = dataVal.toFloat();
              break;
            case HUMIDITY:
              last_Humidity = dataVal.toFloat();
              break;
            case VOLTS:
              last_Voltage = dataVal.toFloat();
              break;
            case WINDOW:
              last_WINDOW = (dataVal.toInt() == 1);
              break;
            case RELAY1:
              last_Relay1 = (dataVal.toInt() == 1);
              break;
            case RELAY2:
              last_Relay2 = (dataVal.toInt() == 1);
              break;
            case EXT_TEMPERATURE:
              last_Ext_Temperature = dataVal.toFloat();
              break;
            case EXT_HUMIDITY:
              last_Ext_Humidity = dataVal.toFloat();
              break;
#endif
            case MILLIS:
              last_Millis = dataVal.toInt();
              break;
            case DATETIME:
              last_DateTime = dataVal;
              break;

            default:
              break;
            }
          }

          if (type == COMMAND)
          {
            switch (dataEnum)
            {
            case WINDOW:
              last_WINDOW = (dataVal.toInt() == 1);
#ifdef Servo_pin
              setWindow(last_WINDOW);
#endif
#if defined(CAMPER)
              // call EXT_SENSORS API to send the command
              callEXT_SENSORSAPI("api/1", String(WINDOW) + "=" + String(last_WINDOW));
#endif
              break;
            case RELAY1:
              last_Relay1 = (dataVal.toInt() == 1);
#ifdef Relay1_pin
              setFan(last_Relay1);
#endif
#if defined(CAMPER)
              // call EXT_SENSORS API to send the command
              callEXT_SENSORSAPI("api/1", String(RELAY1) + "=" + String(last_Relay1));
#endif
              break;
            case RELAY2:
              last_Relay2 = (dataVal.toInt() == 1);
#ifdef Relay2_pin
              setHeater(last_Relay2);
#endif
#if defined(CAMPER)
              // call EXT_SENSORS API to send the command
              callEXT_SENSORSAPI("api/1", String(RELAY2) + "=" + String(last_Relay2));
#endif
              break;
            case DATETIME:
              last_DateTime = dataVal;
              setTime(last_DateTime);
              break;
            }
#if defined(CAMPER)
            // Force a lora send on next loop
            lastLORASend = 0;
#endif
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

    // put module back to listen mode
    radio.startReceive();
    transmitFlag = false;

    // if needed, 'listen' mode can be disabled by calling
    // any of the following methods:
    //
    // radio.standby()
    // radio.sleep()
    // radio.transmit();//I guess adio.startTransmit(); too?
    // radio.receive();
    // radio.readData();
    // radio.scanChannel();

    // we're ready to receive / send more packets,
    // enable interrupt service routine
    interruptEnabled = true;
  }
};

void loraSend(String message)
{
  // check if the last operation is done OR we are in receive mode
  if (loraOperationDone || transmitFlag == false)
  {
    // disable the interrupt service routine while
    // processing the data
    interruptEnabled = false;

    // reset flag
    loraOperationDone = false;
    if (transmitFlag)
    {
      // wait a second before transmitting again, cleanup made on interrupt
      delay(1000);
    }
    // Send message
    radio.startTransmit(message);
    transmitFlag = true;

    Serial.print("sent Message:");
    Serial.println(message);

    // we're ready to send more packets,
    // enable interrupt service routine
    interruptEnabled = true;
    last_SNR = radio.getSNR();
    last_RSSI = radio.getRSSI();
  }
};
#endif