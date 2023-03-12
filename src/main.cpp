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

#include "Sensors.h"

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
Sensors currSensor(32);

void sendWebSocketMessage()
{
  String jsonString = "{";
  jsonString += "\"millis\":" + String(millis()) + ",";
  jsonString += "\"temperature\":" + String(currSensor.getTemperature()) + ",";
  jsonString += "\"humidity\":" + String(currSensor.getHumidity()) + ",";

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
    // TODO: instead of post command
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
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  currSensor.begin();

  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }
  //  Start AP MODE
  WiFi.softAP("ESP32 Camper", "B0bW4lker");
  if (!MDNS.begin("esp32-camper"))
  {
    Serial.println("Error starting mDNS");
    return;
  }
  else
  {
    Serial.println("http://esp32-camper.local registered");
  }
  Serial.println(F(""));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.softAPIP());

  Serial.println(F("Setting handlers"));
  server.serveStatic("/", LittleFS, "/"); // Try the FS first for static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "test connection distance"); });
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
  currSensor.read();

  webSocket.cleanupClients();
}