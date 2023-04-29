#include <Arduino.h>
#include "globals.h"

#include "site.h"
#include "LoraUtils.h"
#ifdef OLED
#include "OledUtil.h"
#endif
#ifdef SENSORS
#include "Sensors.h"
#endif

// WebSocket managed in main code for simpler management
// the main code can send lora message, trigger relays, control servos, etc...
AsyncWebSocket webSocket("/ws");

void sendWebSocketMessage()
{
  String jsonString = "{";
#ifdef SENSORS
  jsonString += "\"millis\":" + String(millis()) + ",";
#else
  jsonString += "\"millis\":" + String(last_Millis) + ",";
#endif
  jsonString += "\"temperature\":" + String(last_Temperature) + ",";
  jsonString += "\"humidity\":" + String(last_Humidity) + ",";
  jsonString += "\"voltage\":" + String(last_Voltage) + ",";
  // TODO: send other data (datetime, window, relays)

  jsonString += "\"dummy\":null}";
  // Serial.println("before webSocket.textAll");

  // webSocket.textAll(jsonString); // send the JSON object through the websocket

  // TODO: Fix issue when webSocket.textAll is called
  /*
  21:12:48.601 >
  21:12:48.601 > assert failed: xQueueSemaphoreTake queue.c:1549 (pxQueue->uxItemSize == 0)
  21:12:48.614 >
  21:12:48.614 >
  21:12:48.614 > Backtrace: 0x40083b99:0x3ffb1fd0 0x4008bed1:0x3ffb1ff0 0x40091d79:0x3ffb2010 0x4008cee1:0x3ffb2140 0x400d66fe:0x3ffb2180 0x400d6cd6:0x3ffb21a0 0x400d760c:0x3ffb21d0 0x400d7631:0x3ffb21f0 0x400d3aea:0x3ffb2210 0x400d3bf8:0x3ffb2270 0x400e5ca5:0x3ffb2290
  21:12:48.630 >
  21:12:48.630 >
  21:12:48.630 >
  21:12:48.630 >
  21:12:48.630 > ELF file SHA256: 09d9805c4891ba04
  */
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
    // TODO: relays if configured
#endif
    LoRaMessage += String(DATETIME) + "=" + String(last_DateTime) + "&";
    LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
    // Serial.println(LoRaMessage);
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

  // Init Wifi
  // TODO: prop / build flag to indicate if use wifi? or always active?
  String SSID = "ESP32 " + String(DEVICE_NAME);
  //  Start AP MODE
  WiFi.softAP(SSID.c_str(), "B0bW4lker"); // TODO: change for prod
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

  // Init Site
  webSocket.onEvent(onWebSocketEvent); // Register WS event handler
  initSite(webSocket);
}

u_long webSockeUpdate = 0;

void loop()
{
  // Update via websocket
  if ((u_long)(millis() - webSockeUpdate) >= 1000)
  {
    sendWebSocketMessage(); // Update the root page with the latest data
    webSockeUpdate = millis();
  }
  // Serial.println("after ws");
#ifdef SENSORS
  readSensors();
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
  // Serial.println("after receive");

  webSocket.cleanupClients();
}