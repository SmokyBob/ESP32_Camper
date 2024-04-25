#include <Arduino.h>
#include "globals.h"

#if defined(CAMPER) || defined(HANDHELD)
#include "LoraUtils.h"
#endif
#ifdef OLED
#include "OledUtil.h"
#endif
#include "Sensors.h"
#if defined(CAMPER) || defined(EXT_SENSORS)
#include "automation.h"
#endif

#ifdef WIFI_PWD
#include "site.h"
#include <DNSServer.h>
#include "esp_wifi.h"
#include "driver/adc.h"
#if defined(CAMPER) || defined(EXT_SENSORS)
#include <HTTPClient.h>
#include <ArduinoJson.h>
#endif
#ifdef BLE_APP
#include "BLEService.h"
#endif

DNSServer dnsServer;
const byte DNS_PORT = 53;

// WebSocket managed in main code for simpler data sharing / code reuse
// the main code can send lora message, trigger relays, control servos, etc...
AsyncWebSocket *webSocket;

void sendWebSocketMessage()
{
  String jsonString = "{";
  for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
  {
    jsonString += "\"" + String(data[i].key) + "\":\"" + data[i].value + "\",";
  }

#if defined(CAMPER)
  jsonString += "\"B_AUTOMATION\":\"" + getConfigVal("B_AUTOMATION") + "\",";
  String tmpBool = "0";
  if (last_IgnoreLowVolt != "")
  {
    tmpBool = "1";
  }

  jsonString += "\"B_VOLT_LIM_IGN\":\"" + String(tmpBool) + "\",";
#endif

  jsonString += "\"dummy\":null}";

  webSocket->textAll(jsonString); // send the JSON object through the websocket
}

void handleWebSocketMessage(void *arg, uint8_t *dataPointer, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    dataPointer[len] = 0;
    String str = (char *)dataPointer;
    Serial.print("      ws command received: ");
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

#if defined(CAMPER)
      if (type == CONFIGS)
      {
        // Send the config to the Ext Sensor via API
        callEXT_SENSORSAPI("api/2", str);
      }
      if (type == DATA)
      {
        // Send the commands to the Ext Sensor via API
        callEXT_SENSORSAPI("api/1", str);
      }

#endif

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

        if (type == DATA)
        {
          // Loop over the data array
          for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
          {
            // Same id, update value
            if (data[i].id == dataEnum)
            {
              data[i].value = dataVal;

              // data with actions
              if (strcmp(data[i].key, "B_WINDOW") == 0)
              {
#ifdef Servo_pin
                setWindow((dataVal == "1"));
#endif
              }
              if (strcmp(data[i].key, "B_FAN") == 0)
              {
#ifdef Relay1_pin
                setFan((dataVal == "1"));
#endif
              }
              if (strcmp(data[i].key, "B_HEATER") == 0)
              {
#ifdef Relay2_pin
                setHeater((dataVal == "1"));
#endif
              }
              if (strcmp(data[i].key, "DATETIME") == 0)
              {
                setDateTime(dataVal);
              }

              break; // found, exit loop
            }
          }

#if defined(CAMPER)
          // Force a lora send on next loop
          lastLORASend = 0;
#elif defined(HANDHELD)
          // send command to CAMPER using lora
          String LoRaMessage = String(DATA) + "?";
          LoRaMessage += String(dataEnum) + "=" + dataVal + "&";
          LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
          // Serial.println(LoRaMessage);
          loraSend(LoRaMessage);
#endif
        }
#if defined(CAMPER) || defined(EXT_SENSORS)
        if (type == CONFIGS)
        { // Loop over the config array
          for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
          {
            // Same id, update value
            if (config[i].id == dataEnum)
            {
              bool storeData = true;
              Serial.printf(" update config id: %u value:%s \n", dataEnum, dataVal);
              // configs with actions
              if (strcmp(config[i].key, "B_VOLT_LIM_IGN") == 0)
              {
                if (dataVal == "1")
                {
                  struct tm timeinfo;
                  getLocalTime(&timeinfo);
                  char buf[100];
                  strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

                  last_IgnoreLowVolt = String(buf);
                }
                else
                {
                  last_IgnoreLowVolt = "";
                }
              }

              if (strcmp(config[i].key, "VOLT_ACTUAL") == 0)
              {
                storeData = false; // don't store the data from the UI, we need to calculate the correct value

                float currVal = dataVal.substring(0, dataVal.indexOf('|')).toFloat();
                float tmpVolt = dataVal.substring(dataVal.indexOf('|') + 1).toFloat();

                // Serial.printf("     voltage currVal: %.2f tmpVolt:%.2f \n", currVal, tmpVolt);
                // Serial.printf("     VDiv_Calibration (str / float): %s / %.2f \n", config[i].value, config[i].value.toFloat());

                if (currVal != tmpVolt)
                {
                  tmpVolt = tmpVolt / config[i].value.toFloat();
                  tmpVolt = currVal / tmpVolt; // calculate the new VDiv_Calibration;

                  // Serial.printf("     stored calibration currVal: %s\n", String(tmpVolt));

                  config[i].value = String(tmpVolt);
                }
              }

              if (storeData)
              {
                config[i].value = dataVal;
              }

              break; // found, exit loop
            }
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
        Serial.println(str);
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
#if defined(CAMPER) || defined(HANDHELD)
    handleWebSocketMessage(arg, data, len);
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
// 0?enum.data.TEMP=36
// 0?enum.data.TEMP=36.00&enum.data.humidity=90.00&enum.data.VOLTS=13.22&enum.data.DATETIME=20230416113532
// 0?enum.data.relay1=0
// RealString
// 0?3=12065&0=20.00&1=35.00&2=13.23&4=20230416113532
// String examples (1 = Commands):
// 1?enum.data.relay1=1
// 1?enum.data.relay1=0
void sendLoRaSensors()
{
#if defined(CAMPER)
  // Send lora only if a client might be listening
  if (last_handheld_hello_millis > 0)
  {
    // Check if the last "hello" was received over the handheld sleep time
    if (millis() > (last_handheld_hello_millis + (HANDHELD_SLEEP_MINS * 60 * 1000)))
    {
      // Wait for the client to be back online
      last_handheld_hello_millis = 0;
      Serial.printf("   handheld shut down, waiting for new \"hello\"\n");
    }
#endif
    // Duty Cycle enforced on sensor data, we ignore it for commands (which go straight to sendLoRaData)
    if ((millis() > (lastLORASend + (LORA_DC * 1000))) && last_handheld_hello_millis > 0)
    {
      String currVal = getDataVal("DATETIME");
      if (currVal.length() > 0)
      {
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        char buf[100];
        strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

        currVal = String(buf);
        setDateTime(currVal); // save the new time
      }
      setDataVal("MILLIS", String(millis()));
      String LoRaMessage = String(DATA) + "?";

      // loop data[] and build the message (some values might get ignored)
      for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
      {
        LoRaMessage += String(data[i].id) + "=" + data[i].value + "&";
      }

      LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
      Serial.println(LoRaMessage);
      loraSend(LoRaMessage);
      lastLORASend = millis();
    }
#if defined(CAMPER)
  }
  else
  {
    // Save time in case of restart
    if (millis() > (lastLORASend + (LORA_DC * 1000)))
    {
      String currVal = getDataVal("DATETIME");
      if (currVal.length() > 0)
      {
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        char buf[100];
        strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

        currVal = String(buf);
        setDateTime(currVal); // save the new time
      }
      lastLORASend = millis();
    }
  }
#endif
}
#endif

void setup()
{
  Serial.begin(115200);

#if defined(CAMPER) || defined(EXT_SENSORS)
  loadPreferences();
#endif
  initSensors();
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

#ifdef BLE_APP
  initBLEService();
#endif

#if defined(HANDHELD)
  // Wake on button
  pinMode(0, INPUT_PULLUP);

#endif
}

u_long webSockeUpdate = 0;
#if defined(CAMPER) || defined(EXT_SENSORS)
unsigned long lastAPICheck = 0;
unsigned long maxAPIPool = 2500;
unsigned long lastLowVolt = 0;
#endif

#if defined(CAMPER) || defined(EXT_SENSORS)
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
    toRet = http.getString();
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
      String currVal = getDataVal("DATETIME");
      if (currVal.length() > 0)
      {
        struct tm timeinfo;
        getLocalTime(&timeinfo);
        char buf[100];
        strftime(buf, sizeof(buf), "%FT%T", &timeinfo);

        currVal = String(buf);
        setDataVal("DATETIME", currVal);
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

  readSensors();

#if defined(CAMPER)
  if (EXT_SENSORS_URL != "")
  {
    if (millis() > lastAPICheck + maxAPIPool)
    {
      // Read sensor data from EXT_SENSORS
      String jsonResult = getUrl(EXT_SENSORS_URL + "/api/sensors");

      Serial.print("api/sensors: ");
      Serial.println(jsonResult);

      if (jsonResult == "")
      {
        // Lost connection with ext_sensor
        // Reinit to reconnect
        EXT_SENSORS_URL = "";
      }
      else
      {
        // with v7 no need to predefine the size
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, jsonResult);

        // Copy values from the JsonDocument
        // only the values from ext_sensors
        String tmp = "";
        String key = "";
        key = "EXT_TEMP";
        tmp = "";
        tmp = doc[key.c_str()].as<String>();
        setDataVal(key.c_str(), tmp);

        key = "EXT_HUM";
        tmp = "";
        tmp = doc[key.c_str()].as<String>();
        setDataVal(key.c_str(), tmp);

        key = "B_WINDOW";
        tmp = "";
        tmp = doc[key.c_str()].as<String>();
        setDataVal(key.c_str(), tmp);

        key = "B_FAN";
        tmp = "";
        tmp = doc[key.c_str()].as<String>();
        setDataVal(key.c_str(), tmp);

        key = "B_HEATER";
        tmp = "";
        tmp = doc[key.c_str()].as<String>();
        setDataVal(key.c_str(), tmp);

        lastAPICheck = millis();
      }
    }
  }

#endif

#if defined(EXT_SENSORS)

  if (millis() > lastAPICheck + maxAPIPool)
  {
    // Read sensor data from CAMPER
    String jsonResult = getUrl(CAMPER_URL + "/api/sensors");

    Serial.print("api/sensors: ");
    Serial.println(jsonResult);

    // with v7 no need to predefine the size
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonResult);

    // Copy values from the JsonDocument
    // only the values from CAMPER
    String key = "VOLTS";
    String tmp = "";
    tmp = doc[key.c_str()].as<String>();
    setDataVal(key.c_str(), tmp);

    lastAPICheck = millis();

    // Check if connected to WIFI
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(F("Wifi Not connected! Reconnecting"));
      ESP.restart(); // Reinit to reconnect
    }
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
  handleLora(); // Always stay in receive mode to check if data/commands have been received
#endif

#if defined(CAMPER) || defined(EXT_SENSORS)
  if (last_IgnoreLowVolt != "")
  {
    // diff with current time, if more than an hour set to "" to stop ignoring the low voltage
    struct tm tm;
    strptime(last_IgnoreLowVolt.c_str(), "%FT%T", &tm);

    time_t time_last = mktime(&tm);
    char buf[100];
    strftime(buf, sizeof(buf), "%FT%T", &tm);
    // Serial.printf("last_IgnoreLowVolt: %s \n", buf);

    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t time_curr = mktime(&timeinfo);

    float minutes_passed = (time_curr - time_last) / 60.0;
    // Serial.printf("mins passed: %.2f \n", minutes_passed);

    if (minutes_passed >= 60.00)
    {
      // 1hr passed, reenable low voltage check
      last_IgnoreLowVolt = "";
    }
  }
  if (last_IgnoreLowVolt == "")
  {
    float voltageLimit = getConfigVal("VOLT_LIM").toFloat();
    if (getDataVal("B_HEATER") == "1")
    {
      // Heater on, check the under load limit
      voltageLimit = getConfigVal("VOLT_LIM_UL").toFloat();
    }
    if (getConfigVal("VOLT_LIM_SL_M").toInt() > 0) // Check only if sleep time is >0 (n.b. set to 0 only for debug to avoid shortening the life of the battery)
    {
      // Sleep for 30 mins if voltage below X volts (defautl 12.0v = 9% for lifepo4 batteries)
      if (getDataVal("VOLTS").toFloat() > 6) //>6 to avoid sleep when connected to the usb for debug
      {
        if (getDataVal("VOLTS").toFloat() < voltageLimit)
        {
          if (lastLowVolt == 0)
          {
            lastLowVolt = millis();
          }
        }
        else
        {
          lastLowVolt = 0;
        }
      }
      else
      {
        lastLowVolt = 0;
      }
      // Go to sleep if undervoltage for more than 30 seconds
      if (lastLowVolt > 0 && (millis() > (lastLowVolt + (30 * 1000))))
      {
        uint64_t hrSleepUs = (1 * (getConfigVal("VOLT_LIM_SL_M").toInt()) * 60 * 1000); // in milliseconds
        hrSleepUs = hrSleepUs * 1000;                                                   // in microseconds
#ifdef WIFI_PWD
        // do many stuff
        WiFi.mode(WIFI_MODE_NULL);
        esp_wifi_stop(); // you must do esp_wifi_start() the next time you'll need wifi or esp32 will crash
#endif
        esp_sleep_enable_timer_wakeup(hrSleepUs);
        esp_deep_sleep_start();
      }
    }
  }
#endif

#if defined(CAMPER) || defined(EXT_SENSORS)
  // Run automation if enabled in settings and we have a voltage value
  if (getConfigVal("B_AUTOMATION") == "1" && getDataVal("VOLTS") != "")
  {
    runAutomation();
  }
#endif

#ifdef BLE_APP
  handleBLE();
#endif

#if defined(HANDHELD)
  // Sleep if awake for more than the configured minutes
  if (millis() > (HANDHELD_AWAKE_MINS * 60 * 1000))
  {
    uint64_t minSleepUs = (HANDHELD_SLEEP_MINS * 60 * 1000 * 1000); // in micro seconds
#ifdef WIFI_PWD
                                                                    // do many stuff
    WiFi.mode(WIFI_MODE_NULL);
    // comment one or both following lines and measure deep sleep current
    esp_wifi_stop(); // you must do esp_wifi_start() the next time you'll need wifi or esp32 will crash
#endif
    Serial.printf("sleep for %u microseconds\n", minSleepUs);
    esp_sleep_enable_timer_wakeup(minSleepUs);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, LOW); // wake on button
    esp_deep_sleep_start();
  }
#endif
}