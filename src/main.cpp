#include <Arduino.h>
#include "globals.h"

#include "LoraUtils.h"
#ifdef OLED
#include "OledUtil.h"
#endif
#ifdef SENSORS
#include "Sensors.h"
#else
#include "RemoteXY.h"
#endif

#ifdef WIFI_PWD
#include "site.h"
#include <DNSServer.h>

DNSServer dnsServer;
const byte DNS_PORT = 53;

// WebSocket managed in main code for simpler data sharing / code reuse
// the main code can send lora message, trigger relays, control servos, etc...
AsyncWebSocket *webSocket;

void sendWebSocketMessage()
{
  String jsonString = "{";
  jsonString += "\"millis\":\"" + String(last_Millis) + "\",";
  jsonString += "\"temperature\":\"" + String(last_Temperature) + "\",";
  jsonString += "\"humidity\":\"" + String(last_Humidity) + "\",";
  jsonString += "\"voltage\":\"" + String(last_Voltage) + "\",";
  jsonString += "\"datetime\":\"" + String(last_DateTime) + "\",";
  jsonString += "\"window\":\"" + String(last_WINDOW) + "\",";
  jsonString += "\"relay1\":\"" + String(last_Relay1) + "\",";
  jsonString += "\"relay2\":\"" + String(last_Relay2) + "\",";
  jsonString += "\"ext_temperature\":\"" + String(last_Ext_Temperature) + "\",";
  jsonString += "\"ext_humidity\":\"" + String(last_Ext_Humidity) + "\",";

  jsonString += "\"dummy\":null}";

  webSocket->textAll(jsonString); // send the JSON object through the websocket
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    sendWebSocketMessage(); // update the webpage accordingly
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    // TODO: instead of post command, manage command received from UI
#ifdef CAMPER
    // TODO: parse and execute the command
    // AwsFrameInfo *info = (AwsFrameInfo *)arg;
    // if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
    // {
    //   data[len] = 0;
    //   message = (char *)data;
    //   steps = message.substring(0, message.indexOf("&"));
    //   direction = message.substring(message.indexOf("&") + 1, message.length());
    //   Serial.print("steps");
    //   Serial.println(steps);
    //   Serial.print("direction");
    //   Serial.println(direction);
    //   notifyClients(direction);
    //   newRequest = true;
    // }
#endif
#ifdef HANDHELD
    // TODO: relay the command with LORA to the CAMPER
#endif

    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}
#endif
unsigned long lastLORASend = 0;
// LoRaData format:
// String examples (0 = Sensor Data):
//  0?enum.data.TEMP=36
//  0?enum.data.TEMP=36.00&enum.data.humidity=90.00&enum.data.VOLTS=13.22&enum.data.DATETIME=20230416113532
//  0?enum.data.relay1=0
// RealString
//  0?3=12065&0=20.00&1=35.00&2=13.23&4=20230416113532
// String examples (1 = Commands):
//  1?enum.data.relay1=1
//  1?enum.data.relay1=0
void sendLoRaSensors()
{
  // Duty Cycle enforced on sensor data, we ignore it for commands (which go straight to sendLoRaData)
  if (millis() > (lastLORASend + (LORA_DC * 1000)))
  {
    String LoRaMessage = String(DATA) + "?";
    LoRaMessage += String(MILLIS) + "=" + String(millis()) + "&";
#ifdef SENSORS
    LoRaMessage += String(TEMPERATURE) + "=" + String(last_Temperature) + "&";
    LoRaMessage += String(HUMIDITY) + "=" + String(last_Humidity) + "&";
    LoRaMessage += String(VOLTS) + "=" + String(last_Voltage) + "&";
    LoRaMessage += String(WINDOW) + "=" + String(last_WINDOW) + "&";
    LoRaMessage += String(RELAY1) + "=" + String(last_Relay1) + "&";
    LoRaMessage += String(RELAY2) + "=" + String(last_Relay2) + "&";
    if (isnan(last_Ext_Temperature) != true)
    {
      LoRaMessage += String(EXT_TEMPERATURE) + "=" + String(last_Ext_Temperature) + "&";
    }
    if (isnan(last_Ext_Humidity) != true)
    {
      LoRaMessage += String(EXT_HUMIDITY) + "=" + String(last_Ext_Humidity) + "&";
    }
#endif
    LoRaMessage += String(DATETIME) + "=" + String(last_DateTime) + "&";
    LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
    Serial.println(LoRaMessage);
    loraSend(LoRaMessage);
    lastLORASend = millis();
  }
}

void setup()
{
  Serial.begin(115200);

#ifdef SENSORS
  initSensors();
#endif
#ifdef OLED
  initOled();
#endif
  initLora();

#ifdef WIFI_PWD
  // Init Wifi
  String SSID = "ESP32 " + String(DEVICE_NAME);
  //  Start AP MODE
  WiFi.softAP(SSID.c_str(), String(WIFI_PWD));
  String tmpDN = "esp32-" + String(DEVICE_NAME);
  if (!MDNS.begin(tmpDN.c_str()))
  {
    Serial.println("Error starting mDNS");
    return;
  }
  else
  {
    Serial.println("http://" + tmpDN + ".local registered");
  }
  Serial.println(F(""));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Init Websocket
  webSocket = new AsyncWebSocket("/ws");
  webSocket->onEvent(onWebSocketEvent); // Register WS event handler

  // init the rest of the site
  initSite(webSocket);
#endif
}

u_long webSockeUpdate = 0;

void loop()
{
#ifdef WIFI_PWD
  // DNS
  dnsServer.processNextRequest(); // Captive Portal

  // Update via websocket
  if ((u_long)(millis() - webSockeUpdate) >= 1000)
  {
    sendWebSocketMessage(); // Update the root page with the latest data
    webSockeUpdate = millis();
  }
  // Serial.println("after ws");

  webSocket->cleanupClients();
#endif
#ifdef SENSORS
  readSensors();
  float currTemp;
  currTemp = last_Temperature;
#ifdef EXT_DHT22_pin
  if (isnan(last_Ext_Temperature) != true)
  {
    currTemp = last_Ext_Temperature;
  }
#endif
#ifdef Servo_pin
  // TODO: use Automation Code; this is just a test
  if (currTemp > 30)
  {
    if (!last_WINDOW)
    {
      // Open the window
      setWindow(true);
    }
  }

  if (currTemp < 20)
  {
    if (last_WINDOW)
    {
      // Close the window
      setWindow(false);
    }
  }
#endif

#endif

#if OLED
  // update display with data of the current page
  drawPage();
#endif
  // Serial.println("after OLED");

#ifdef CAMPER
  sendLoRaSensors();
#endif
  loraReceive(); // Always stay in receive mode to check if data/commands have been received

#ifdef Voltage_pin
  // TODO: configurable in parameters showing the percent table as reference
  float voltageLimit = 12.5;
  // Sleep for 30 mins if voltage below 12.5v (14% for lifepo4 batteries)
  if (last_Voltage > 6 && last_Voltage < voltageLimit) //>6 to avoid sleep when connected to the usb for debug
  {
    uint64_t hrSleepUs = (1 * 60 * 60 * 1000); // in milliseconds
    hrSleepUs = hrSleepUs * 1000;              // in microseconds
    esp_sleep_enable_timer_wakeup(hrSleepUs);
    esp_deep_sleep_start();
  }
#endif
  // Serial.println("after receive");
}