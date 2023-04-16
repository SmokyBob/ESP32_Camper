#include <Arduino.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "enums.h"
#include "shared.h"
#ifdef SENSORS
#include "Sensors.h"
#endif

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");

shared utils;
#ifdef SENSORS
Sensors currSensor(DHT11_pin, Voltage_pin);
#endif

void sendWebSocketMessage()
{
  String jsonString = "{";
#ifdef SENSORS
  jsonString += "\"millis\":" + String(millis()) + ",";
  jsonString += "\"temperature\":" + String(currSensor.temperature) + ",";
  jsonString += "\"humidity\":" + String(currSensor.humidity) + ",";
  jsonString += "\"voltage\":" + String(currSensor.voltage) + ",";
#else
  jsonString += "\"millis\":" + String(lastLoraMillis) + ",";
  jsonString += "\"temperature\":" + String(lastTemp) + ",";
  jsonString += "\"humidity\":" + String(lastHum) + ",";
  jsonString += "\"voltage\":" + String(lastVoltage) + ",";
#endif

  jsonString += "\"dummy\":null}";
  webSocket.textAll(jsonString); // send the JSON object through the websocket
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
//  0?enum.data.TEMP=36&enum.data.humidity=90&enum.data.VOLTS=13.22&enum.data.DATETIME=20230416113532
//  0?enum.data.relay1=0
// RealString
//  0?0=20&1=35&2=13.23&3=12065&4=20230416113532
// String examples (1 = Commands):
//  1?enum.data.relay1=1
//  1?enum.data.relay1=0
void sendLoRaSensors()
{
  // Duty Cycle enforced on sensor data, we ignore it for commands (which go straight to sendLoRaData)
  if (millis() > (lastLORASend + (LORA_DC * 1000)))
  {
    String LoRaMessage = String(DATA) + "?" + String(MILLIS) + "=" + String(millis()) + "&";
#ifdef SENSORS
    LoRaMessage += String(DATA) + "?" + String(TEMPERATURE) + "=" + String(currSensor.temperature) + "|";
    LoRaMessage += String(DATA) + "?" + String(HUMIDITY) + "=" + String(currSensor.humidity) + "|";
    LoRaMessage += String(DATA) + "?" + String(VOLTS) + "=" + String(currSensor.voltage) + "|";
#endif
    // TODO: relays if configured
    LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
    utils.loraSend(LoRaMessage);
  }
}

void setup()
{
  Serial.begin(115200);
#ifdef SENSORS
  currSensor.begin();
#endif

  utils.begin(); // init oled,lora, etc.. as needed

  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }
  String SSID = "ESP32 " + String(DEVICE_NAME);
  //  Start AP MODE
  WiFi.softAP(SSID.c_str(), "B0bW4lker");
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

  Serial.println(F("Setting handlers"));
  server.serveStatic("/", LittleFS, "/"); // Try the FS first for static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });
  // TODO: commands endpoint
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "File not found"); });
  webSocket.onEvent(onWebSocketEvent); // Register WS event handler
  server.addHandler(&webSocket);
  server.begin();
}

unsigned long webSockeUpdate = 0;

void loop()
{
  // put your main code here, to run repeatedly:
  if ((unsigned long)(millis() - webSockeUpdate) >= 1000)
  {
    sendWebSocketMessage();    // Update the root page with the latest data
    webSockeUpdate = millis(); // Use the snapshot to set track time until next event
  }
#ifdef SENSORS
  currSensor.read();
#endif

#ifdef CAMPER
  sendLoRaSensors();
#endif
  utils.loraReceive();//Always stay check if data has been received

  webSocket.cleanupClients();
}