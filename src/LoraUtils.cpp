#include "Arduino.h"
#include "LoraUtils.h"

#ifdef E220
// TODO: E220 defs
#else
SX1276 radio = nullptr;
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
#endif

float last_Temp;
float last_Hum;
float last_Voltage;
bool last_WINDOW;
bool last_Relay1;
bool last_Relay2;
u_long last_Millis;
String last_DateTime;

float last_SNR;
float last_RSSI;

void initLora()
{

// LORA INIT
#ifdef E220
  // TODO: E220

#else
  radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

  // SPI LoRa pins
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
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
    radio.setCurrentLimit(120);

    // set the function that will be called
    // when new packet is received/transmitted
    radio.setDio0Action(setLoraFlags);

// different initial state for CAMPER and HANDHELD
#ifdef CAMPER
    Serial.print(F("[SX1276] Starting to transmit ... "));
    loraState = radio.startTransmit(String(DEVICE_NAME) + " online");
    transmitFlag = true;
#endif
#ifdef HANDHELD
    loraState = radio.startReceive();
    transmitFlag = false;
#endif
  }
#endif
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
#ifdef E220
// TODO:.... Radio lib SHOULD support lcXXXX chip... maybe it's just a matter on init?
#else
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
    if (state == RADIOLIB_ERR_NONE)
    {
      // Ex. 0?0=20&1=35&2=13.23&3=12065&4=20230416113532
      int posCommand = str.indexOf('?');

      int type = str.substring(0, posCommand).toInt();
      // Remove type from string
      str = str.substring(posCommand + 1);
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

        switch (dataEnum)
        {
#ifndef SENSORS
        case TEMPERATURE:
          last_Temp = dataVal.toFloat();
          break;
          // TODO: altri Case dei sensori
#endif
        case MILLIS:
          last_Millis = dataVal.toInt();
          break;
        case DATETIME:
          last_DateTime = dataVal;
          break;
          // TODO: altri Case

        default:
          break;
        }

        if (type == COMMAND)
        {
          // TODO: eseguire il comando e poi accodare "subito" l'invio di un messaggio
        }

        // Remove the read data from the message
        if (idxValEnd > 0)
        {
          dataVal = str.substring(idxValEnd + 1);
        }
        else
        {
          dataVal = "";
        }

      } while (str.indexOf('&') > 0);

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
#endif
};

void loraSend(String message)
{
#ifdef E220
// TODO:.... Radio lib SHOULD support lcXXXX chip... maybe it's just a matter on init?
#else
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
    // we're ready to send more packets,
    // enable interrupt service routine
    interruptEnabled = true;
  }
#endif
};