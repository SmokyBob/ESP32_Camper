#pragma once
#include "Arduino.h"
#include <ESPmDNS.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#if defined(CAMPER) || defined(HANDHELD)
#include <LittleFS.h>
#endif

#if defined(CAMPER) || defined(EXT_SENSORS) || defined(CAR)
#include "Sensors.h"
#include "automation.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#endif

void initSite(AsyncWebSocket *webSocket);

#if defined(CAMPER) || defined(EXT_SENSORS)
String getUrl(String ReqUrl);
#endif

#if defined(CAMPER)
void callEXT_API(String rawUrl, String payload);
#endif
