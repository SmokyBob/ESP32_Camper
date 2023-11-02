#include <Arduino.h>
#include "globals.h"

#if defined(CAMPER) || defined(HANDHELD)
#include "LoraUtils.h"
#endif
#ifdef OLED
#include "OledUtil.h"
#endif
#if defined(CAMPER) || defined(EXT_SENSORS)
#include "Sensors.h"
#include "automation.h"
#endif

#ifdef WIFI_PWD
#include "site.h"
#include <DNSServer.h>
#include "esp_wifi.h"
#include "driver/adc.h"
#if defined(CAMPER) || defined(EXT_SENSORS)
#include <HTTPClient.h>
#endif
#if defined(CAMPER)
#include <ArduinoJson.h>
#endif

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
  jsonString += "\"ext_temperature\":\"" + String(last_Ext_Temperature) + "\",";
  jsonString += "\"ext_humidity\":\"" + String(last_Ext_Humidity) + "\",";
  jsonString += "\"voltage\":\"" + String(last_Voltage) + "\",";
  jsonString += "\"datetime\":\"" + String(last_DateTime) + "\",";
  jsonString += "\"window\":\"" + String(last_WINDOW) + "\",";
  jsonString += "\"relay1\":\"" + String(last_Relay1) + "\",";
  jsonString += "\"relay2\":\"" + String(last_Relay2) + "\",";

  jsonString += "\"dummy\":null}";

  webSocket->textAll(jsonString); // send the JSON object through the websocket
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    String str = (char *)data;
    Serial.print("ws command received: ");
    Serial.println(str);

#if defined(CAMPER) || defined(EXT_SENSORS)
    if (str == "resetParams")
    {
      resetPreferences();
      return;
    }
#endif
    int posCommand = str.indexOf('?');
    if (posCommand > 0)
    {

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

        if (type == COMMAND)
        {
          switch (dataEnum)
          {
          case WINDOW:
            last_WINDOW = (dataVal.toInt() == 1);
#ifdef Servo_pin
            setWindow(last_WINDOW);
#endif
            break;
          case RELAY1:
            last_Relay1 = (dataVal.toInt() == 1);
#ifdef Relay1_pin
            setFan(last_Relay1);
#endif
            break;
          case RELAY2:
            last_Relay2 = (dataVal.toInt() == 1);
#ifdef Relay2_pin
            setHeater(last_Relay2);
#endif
            break;
          case DATETIME:
            last_DateTime = dataVal;
            setTime(last_DateTime);
            break;
          }
#if defined(CAMPER)
          // Force a lora send on next loop
          lastLORASend = 0;
          // TODO: send command to EXT_SENSORS via API call
          // call EXT_SENSORS API to send the command
          // callEXT_SENSORSAPI("api/1", String(RELAY1) + "=" + String(last_Relay1));
#elif defined(HANDHELD)
          // TODO: send command to CAMPER using lora
#endif
        }
#if defined(CAMPER) || defined(EXT_SENSORS)
        if (type == CONFIGS)
        {
          switch (dataEnum)
          {
          case CONFIG_SERVO_CLOSED_POS:
            settings[0].value = dataVal.toFloat();
            break;
          case CONFIG_SERVO_OPEN_POS:
            settings[1].value = dataVal.toFloat();
            break;
          case CONFIG_SERVO_CLOSED_TEMP:
            settings[2].value = dataVal.toFloat();
            break;
          case CONFIG_SERVO_OPEN_TEMP:
            settings[3].value = dataVal.toFloat();
            break;
          case CONFIG_VOLTAGE_ACTUAL:
          {
            float currVal = dataVal.substring(0, dataVal.indexOf('|')).toFloat();
            float tmpVolt = dataVal.substring(dataVal.indexOf('|') + 1).toFloat();

            if (currVal != tmpVolt)
            {
              tmpVolt = tmpVolt / settings[4].value;

              settings[4].value = currVal / tmpVolt;
            }
            break;
          }
          case CONFIG_VOLTAGE_LIMIT:
            settings[5].value = dataVal.toFloat();
            break;
          case CONFIG_VOLTAGE_LIMIT_UNDER_LOAD:
            settings[6].value = dataVal.toFloat();
            break;
          case CONFIG_VOLTAGE_SLEEP_MINUTES:
            settings[7].value = dataVal.toFloat();
            break;
          case CONFIG_ENABLE_AUTOMATION:
            Serial.print("CONFIG_ENABLE_AUTOMATION:");
            Serial.println(dataVal);
            settings[8].value = dataVal.toFloat();
            break;
            ;
          }
          savePreferences();
        }
#endif

        // Remove the read data from the message
        if (idxValEnd > 0)
        {
          str = str.substring(idxValEnd + 1);
        }
        else
        {
          str = "";
        }
        // Serial.println(str);
      } while (str.length() > 0);
    }
  }
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
#ifdef CAMPER
    handleWebSocketMessage(arg, data, len);
#endif
#ifdef HANDHELD
    // TODO: relay the command with LORA to the CAMPER
#endif

    break;
  case WS_EVT_PONG:
    break;
  case WS_EVT_ERROR:
    break;
  }
}
#endif

#if defined(CAMPER) || defined(HANDHELD)
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
    if (last_DateTime.length() > 0)
    {
      struct tm timeinfo;
      getLocalTime(&timeinfo);
      char buf[100];
      strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

      last_DateTime = String(buf);
    }
    String LoRaMessage = String(DATA) + "?";
    LoRaMessage += String(MILLIS) + "=" + String(millis()) + "&";
#if defined(CAMPER)
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
#endif

void setup()
{
  Serial.begin(115200);

#if defined(CAMPER) || defined(EXT_SENSORS)
  loadPreferences();
  initSensors();
#endif
#ifdef OLED
  initOled();
#endif
#if defined(CAMPER) || defined(HANDHELD)
  initLora();
#endif

#ifdef WIFI_PWD

#if defined(CAMPER) || defined(HANDHELD)
  // Init Wifi
  String SSID = "ESP32 " + String(DEVICE_NAME);
  //  Start AP MODE
  WiFi.softAP(SSID.c_str(), String(WIFI_PWD));
  // esp_wifi_start();
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
#else

  // Connect to the Camper Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(String("ESP32 CAMPER").c_str(), String(WIFI_PWD).c_str());

  unsigned long connectTimeout = millis();
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
    if ((millis() - connectTimeout) > (5 * 60 * 1000))
    {
      Serial.println("Wifi Not connected with ssid: "
                     "ESP32 CAMPER"
                     " ! Force AP Mode");
      break;
    }
  }
  setWindow(false); // reset the window to closed
  Serial.println(F(""));
  Serial.println(F("IP address: "));
  Serial.println(WiFi.localIP());

#endif

#if defined(CAMPER) || defined(HANDHELD)
  // Init Websocket
  webSocket = new AsyncWebSocket("/ws");
  webSocket->onEvent(onWebSocketEvent); // Register WS event handler
#endif

  // init the rest of the site / API /OTA
  initSite(webSocket);

#endif
}

u_long webSockeUpdate = 0;
#if defined(CAMPER)
unsigned long lastAPICheck = 0;
unsigned long maxAPIPool = 2500;
#endif

#if defined(CAMPER)
String getUrl(String ReqUrl)
{
  String toRet = "";
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(ReqUrl.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    // Serial.print("HTTP Response code: ");
    // Serial.println(httpResponseCode);
    String payload = http.getString();
    // Serial.println("result: ");
    // Serial.println(payload);
    // Serial.println("");
    // Serial.println("------");
    toRet = payload;
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return toRet;
}
#endif

void loop()
{

#ifdef WIFI_PWD
#if defined(CAMPER) || defined(HANDHELD)
  // DNS
  dnsServer.processNextRequest(); // Captive Portal

  // Update via websocket
  if ((u_long)(millis() - webSockeUpdate) >= 1000)
  {
    if (webSocket->getClients().length() > 0)
    {
      if (last_DateTime.length() > 0)
      {
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        char buf[100];
        strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

        last_DateTime = String(buf);
      }
      sendWebSocketMessage(); // Update the root page with the latest data
    }
    webSockeUpdate = millis();
  }
  // Serial.println("after ws");

  webSocket->cleanupClients();
#endif
#if defined(CAMPER)
  if (EXT_SENSORS_URL == "")
  {
    // Search connected devices
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;

    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

    for (int i = 0; i < adapter_sta_list.num; i++)
    {

      tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];

      // Serial.print("station nr ");
      // Serial.println(i);

      // Serial.print("MAC: ");

      // for (int i = 0; i < 6; i++)
      // {

      //   Serial.printf("%02X", station.mac[i]);
      //   if (i < 5)
      //     Serial.print(":");
      // }

      Serial.print("\nIP: ");
      char str_ip[16];
      esp_ip4addr_ntoa(&station.ip, str_ip, IP4ADDR_STRLEN_MAX);
      Serial.println(str_ip);

      // Test the API Get
      String testURL = "http://" + String(str_ip) + "/api/sensors";

      // Serial.print("testURL: ");
      // Serial.println(testURL);

      String tmpRes = getUrl(testURL);
      if (tmpRes.length() != 0)
      {
        // Got the result, save the base address for future calls
        EXT_SENSORS_URL = "http://" + String(str_ip);
        Serial.print("EXT_SENSORS_URL :");
        Serial.println(EXT_SENSORS_URL);
      }
    }
  }
#endif
#endif
#if defined(CAMPER) || defined(EXT_SENSORS)
  readSensors();
#endif

#if defined(CAMPER)
  if (EXT_SENSORS_URL != "")
  {
    if (millis() > lastAPICheck + maxAPIPool)
    {
      // Read sensor data from EXT_SENSORS
      String jsonResult = getUrl(EXT_SENSORS_URL + "/api/sensors");

      Serial.print("api/sensors: ");
      Serial.println(jsonResult);

      // Allocate a temporary JsonDocument
      // Don't forget to change the capacity to match your requirements.
      // Use https://arduinojson.org/v6/assistant to compute the capacity.
      StaticJsonDocument<384> doc;
      DeserializationError error = deserializeJson(doc, jsonResult);

      // Copy values from the JsonDocument
      //only the values from ext_sensors
      last_Ext_Temperature = doc["ext_temperature"];
      last_Ext_Humidity = doc["ext_humidity"];
      last_WINDOW = doc["window"];
      last_Relay1 = doc["relay1"];
      last_Relay2 = doc["relay2"];
      
      lastAPICheck = millis();
    }
  }

#endif

#if defined(CAMPER) || defined(EXT_SENSORS)
  // Run automation if enabled in settings
  if (settings[8].value > 0.00)
  {
    runAutomation();
  }
#endif

#if OLED
  // update display with data of the current page
  drawPage();
#endif
  // Serial.println("after OLED");

#ifdef CAMPER
  sendLoRaSensors();
#endif

#if defined(CAMPER) || defined(HANDHELD)
  loraReceive(); // Always stay in receive mode to check if data/commands have been received
#endif

#ifdef Voltage_pin
  float voltageLimit = settings[5].value;
  if (last_Relay2) {
    //Heater on, check the under load limit
    voltageLimit = settings[6].value;
  }
  if (settings[7].value > 0) // Check only if sleep time is >0 (n.b. set to 0 only for debug to avoid shortening the life of the battery)
  {
    // Sleep for 30 mins if voltage below X volts (defautl 12.0v = 9% for lifepo4 batteries)
    if (last_Voltage > 6 && last_Voltage < voltageLimit) //>6 to avoid sleep when connected to the usb for debug
    {
      uint64_t hrSleepUs = (1 * (settings[7].value) * 60 * 1000); // in milliseconds
      hrSleepUs = hrSleepUs * 1000;                               // in microseconds
#ifdef WIFI_PWD
      // do many stuff
      WiFi.mode(WIFI_MODE_NULL);
      // comment one or both following lines and measure deep sleep current
      esp_wifi_stop(); // you must do esp_wifi_start() the next time you'll need wifi or esp32 will crash
      adc_power_release();
#endif
      esp_sleep_enable_timer_wakeup(hrSleepUs);
      esp_deep_sleep_start();
    }
  }
#endif
  // Serial.println("after receive");
}