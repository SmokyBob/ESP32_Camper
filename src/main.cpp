#include <Arduino.h>
#include <ESPmDNS.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
  //  Start AP MODE
  WiFi.softAP("ESP32 Camper", "B0bW4lker");
  if(!MDNS.begin("esp32-camper")) {
     Serial.println("Error starting mDNS");
     return;
  }else {
    Serial.println("http://esp32-camper.local registered");
  }
  Serial.println(F(""));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.softAPIP());

  Serial.println(F("Setting handlers"));
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "test connection distance"); });
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "test connection distance"); });
  server.begin();
}

void loop()
{
  // put your main code here, to run repeatedly:
}