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
#ifdef SENSORS
#include "Sensors.h"
#endif

#ifdef E220
#include "LoRa_E220.h"
#endif

#ifdef OLED
// Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

// Libraries for OLED Display
#include <Wire.h>
#include "SSD1306.h"

// define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America
#define BAND 866E6

// OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

SSD1306 display(0x3c, OLED_SDA, OLED_SCL, GEOMETRY_128_64);

#endif

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");

#ifdef E220
LoRa_E220 e220ttl(&Serial2); // WeMos RX --> e220 TX - WeMos TX --> e220 RX
#endif
#ifdef SENSORS
Sensors currSensor(32, 34, 30);
#else
unsigned long lastLoraMillis;
float lastTemp;
float lastHum;
int lastWatt;
float lastAmps;
#endif

void sendWebSocketMessage()
{
  String jsonString = "{";
#ifdef SENSORS
  jsonString += "\"millis\":" + String(millis()) + ",";
  jsonString += "\"temperature\":" + String(currSensor.temperature) + ",";
  jsonString += "\"humidity\":" + String(currSensor.humidity) + ",";
  jsonString += "\"watt\":" + String(currSensor.Watt) + ",";
  jsonString += "\"amps\":" + String(currSensor.AmpsRMS) + ",";
#else
  jsonString += "\"millis\":" + String(lastLoraMillis) + ",";
  jsonString += "\"temperature\":" + String(lastTemp) + ",";
  jsonString += "\"humidity\":" + String(lastHum) + ",";
  jsonString += "\"watt\":" + String(lastWatt) + ",";
  jsonString += "\"amps\":" + String(lastAmps) + ",";
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
    // TODO: parse command string
#ifdef CAMPER
    // TODO: elaborate the command
#endif
#ifdef HANDHELD
    // TODO: relay the command with LORA to the CAMPER
#endif

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

#ifdef OLED
void startOLED()
{
  // reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // initialize OLED
  display.init();

  // display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setColor(BLACK); // Display color
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.clear();
#ifdef HANDHELD
  display.drawString(0, 0, "LORA RECEIVER");
#endif
#ifdef CAMPER
  display.drawString(0, 0, "LORA SENDER");
#endif
}
#endif

// Initialize LoRa module
void startLoRA()
{
  int counter;
  String mex;
#ifndef E220
  // SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  // setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10)
  {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10)
  {
    // Increment readingID on every new reading
    mex = "Starting LoRa failed!";
    Serial.println(mex);
  }
  else
  {
    mex = "LoRa Initialization OK!";
    Serial.println(mex);
  }
#else
  e220ttl.begin();

  Serial.println("lora E220 test send");
  ResponseStatus rs = e220ttl.sendMessage("test message on init");
  // Check If there is some problem of successfully send
  Serial.println(rs.getResponseDescription());
  // TODO: check the e220 message result to decide if YaY or Nay
#endif
#ifdef OLED
  display.clear();
  display.setFont(ArialMT_Plain_10);
#ifdef HANDHELD
  display.drawString(0, 0, "LORA RECEIVER");
#endif
#ifdef CAMPER
  display.drawString(0, 0, "LORA SENDER");
#endif
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, mex);
  display.display();
#endif
  delay(200);
}

int rssi;
String loRaMessage;
String temperature;
String humidity;
String readingID;

// LoRaData format: Type&key=val
// String examples (0 = Sensor Data):
//  0&temp=36
//  0&hum=90
//  0&watt=10
//  0&amp=0.5
//  0&relay1=LOW
// String examples (1 = Commands):
//  1&relay1=HIGH
//  1&relay1=LOW

// Read LoRa packet and get the sensor readings
void getLoRaData()
{
  Serial.print("Lora packet received: ");
#ifdef OLED
  display.clear();
  display.setFont(ArialMT_Plain_10);
#ifdef HANDHELD
  display.drawString(0, 0, "LORA RECEIVER");
#endif
#ifdef CAMPER
  display.drawString(0, 0, "LORA SENDER");
#endif
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Lora packet received: ");
#endif
  String LoRaData;

#ifdef E220
  // Read packet
  while (e220ttl.available())
  {
    // read the String message
    ResponseContainer rc = e220ttl.receiveMessage();
    LoRaData = rc.data;
#else
  // Read packet
  while (LoRa.available())
  {
    LoRaData = LoRa.readString();
#endif
    // TODO: parse LoRa Data to identify the type

    Serial.print(LoRaData);
#ifdef OLED
    display.print(LoRaData);
#endif

    // // Get readingID, temperature and humidity
    // int pos1 = LoRaData.indexOf('/');
    // int pos2 = LoRaData.indexOf('&');
    // int pos3 = LoRaData.indexOf('#');
    // readingID = LoRaData.substring(0, pos1);
    // temperature = LoRaData.substring(pos1 +1, pos2);
    // humidity = LoRaData.substring(pos2+1, pos3);
  }
#ifndef E220
  // Get RSSI
  rssi = LoRa.packetRssi();
  Serial.print(" with RSSI ");
  Serial.println(rssi);
#ifdef OLED
  display.print(" with RSSI ");
  display.print(rssi);
  display.display();
#endif
#endif
}

// LoRaData format: Type&key=val
// String examples (0 = Sensor Data):
//  0&temp=36
//  0&hum=90
//  0&watt=10
//  0&amp=0.5
//  0&relay1=LOW
// String examples (1 = Commands):
//  1&relay1=HIGH
//  1&relay1=LOW
void sendLoRaSensors()
{
  // TODO: get sensor data
  String LoRaMessage = String(millis()) + "&" + String(1) + "#" + String(2);
  sendLoRaData(LoRaMessage);
}

void sendLoRaData(String command)
{
#ifdef E220
  e220ttl.sendMessage(command); // TODO: l'altro legge RSSI... devo usare la firma che lo aggiunge sempre?
#else
  // Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(command);
  LoRa.endPacket();
#endif
}

void setup()
{
  Serial.begin(115200);
#ifdef SENSORS
  currSensor.begin();
#endif

#ifdef OLED
  startOLED();
#endif

  startLoRA();

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
#ifdef SENSORS
    sendWebSocketMessage(); // Update the root page with the latest data
#endif
    webSockeUpdate = millis(); // Use the snapshot to set track time until next event
  }
#if defined(SENSORS)
  currSensor.read();
#endif
  getLoRaData();

#ifdef CAMPER
  sendLoRaSensors();
#endif

  webSocket.cleanupClients();
}