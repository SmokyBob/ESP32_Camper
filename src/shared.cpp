#include "Arduino.h"
#include "shared.h"

#ifdef E220
// TODO: E220 defs
#else

void shared::setFlags()
{
  // https://github.com/jgromes/RadioLib/discussions/287#discussioncomment-2463000
  shared instance = shared::getInstance();
  if (!instance.interruptEnabled)
  {
    return;
  }
  // TODO: test if this "if" works
  //  if we were transmitting, cleanup and delay are needed
  if (instance.transmitFlag)
  {

    //  clean up after transmission is finished
    //  this will ensure transmitter is disabled,
    //  RF switch is powered down etc.
    instance.radio.finishTransmit();
  }
  // we sent or received a packet, set the flag
  instance.loraOperationDone = true;
}
#endif

shared::shared()
{
}

shared::~shared()
{
}

void shared::begin()
{
#ifdef OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  // reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(20);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  delay(20);

  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
  {
    Serial.println("Started OLED");
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

    u8g2->begin();
    u8g2->clearBuffer();
    u8g2->setFlipMode(1); // 180 Degree flip... because i like the buttons on the right side
    u8g2->setFontMode(1); // Transparent
    u8g2->setDrawColor(1);
    u8g2->setFontDirection(0);

    u8g2->setFont(u8g2_font_unifont_tr);
    u8g2->drawStr(0, (1 * 10) + (2 * (1 - 1)), device_name.c_str());
    u8g2->drawStr(0, (2 * 10) + (2 * (2 - 1)), "Starting ...");

    u8g2->sendBuffer();

    delay(3000);
  }
#endif
// LORA INIT
#ifdef E220
  // TODO: E220

#else
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
#ifdef OLED
    u8g2->clearBuffer();
    u8g2->drawStr(0, 12, "Initializing: FAIL!");
    u8g2->sendBuffer();
#endif
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
    radio.setDio0Action(setFlags); // https://github.com/jgromes/RadioLib/discussions/287#discussioncomment-2463000

// different initial state for CAMPER and HANDHELD
#ifdef CAMPER
    Serial.print(F("[SX1276] Starting to transmit ... "));
    loraState = radio.startTransmit(device_name + " online");
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
void shared::loraReceive()
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
void shared::loraSend(String message)
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