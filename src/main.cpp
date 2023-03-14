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
#include "LoRa_E220.h"
#endif

#ifdef HANDHELD
// Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

// Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

#endif

AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
#ifdef SENSORS
Sensors currSensor(32);
LoRa_E220 e220ttl(&Serial2); // WeMos RX --> e220 TX - WeMos TX --> e220 RX

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
#endif

#ifdef HANDHELD
void startOLED()
{
  // reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LORA RECEIVER");
}

// Initialize LoRa module
void startLoRA()
{
  int counter;
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
    Serial.println("Starting LoRa failed!");
  }
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0, 10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

int rssi;
String loRaMessage;
String temperature;
String humidity;
String readingID;

// Read LoRa packet and get the sensor readings
void getLoRaData()
{
  display.setCursor(0, 10);
  display.clearDisplay();
  Serial.print("Lora packet received: ");
  display.print("Lora packet received: ");
  // Read packet
  while (LoRa.available())
  {
    String LoRaData = LoRa.readString();
    // LoRaData format: readingID/temperature&soilMoisture#batterylevel
    // String example: 1/27.43&654#95.34
    Serial.print(LoRaData);
    display.print(LoRaData);

    // // Get readingID, temperature and humidity
    // int pos1 = LoRaData.indexOf('/');
    // int pos2 = LoRaData.indexOf('&');
    // int pos3 = LoRaData.indexOf('#');
    // readingID = LoRaData.substring(0, pos1);
    // temperature = LoRaData.substring(pos1 +1, pos2);
    // humidity = LoRaData.substring(pos2+1, pos3);
  }
  // Get RSSI
  rssi = LoRa.packetRssi();
  Serial.print(" with RSSI ");
  display.print(" with RSSI ");
  Serial.println(rssi);
  display.print(rssi);
  display.display();
}
#endif

void setup()
{
  Serial.begin(115200);
#ifdef SENSORS
  currSensor.begin();
  e220ttl.begin();

  Serial.println("lora test send");
  ResponseStatus rs = e220ttl.sendMessage("Hello, world?");
  // Check If there is some problem of successfully send
  Serial.println(rs.getResponseDescription());
#endif

#ifdef HANDHELD
  startOLED();
  startLoRA();
#endif

  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }
  String SSID = "ESP32 " + String(DEVICE_NAME);
  //  Start AP MODE
  WiFi.softAP(SSID.c_str(), "B0bW4lker");
  String tmpDN = "esp32-"+ String(DEVICE_NAME);
  if (!MDNS.begin(tmpDN.c_str()))
  {
    Serial.println("Error starting mDNS");
    return;
  }
  else
  {
    Serial.println("http://"+tmpDN+".local registered");
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
#ifdef SENSORS
  webSocket.onEvent(onWebSocketEvent); // Register WS event handler
#endif
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
#ifdef SENSORS
  currSensor.read();

  // If something available
  if (e220ttl.available() > 1)
  {
    // read the String message
    ResponseContainer rc = e220ttl.receiveMessage();
    // Is something goes wrong print error
    if (rc.status.code != 1)
    {
      rc.status.getResponseDescription();
    }
    else
    {
      // Print the data received
      Serial.println(rc.data);
    }
  }
  
  //TODO: test sending messages so that the other code can read them
  if ((unsigned long)(millis() - webSockeUpdate) >= 1000)
  {

    e220ttl.sendMessage("test lora e200");
  }

#endif
#ifdef HANDHELD
  // Check if there are LoRa packets available
  int packetSize = LoRa.parsePacket();
  if (packetSize)
  {
    getLoRaData();
    // getTimeStamp();
  }
#endif

  webSocket.cleanupClients();
}