#include "site.h"
#include "globals.h"
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);

void api_get(AsyncWebServerRequest *request)
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

  Serial.print("api get: ");
  Serial.println(jsonString);

  request->send(200, "application/json", jsonString);
}

#if defined(CAMPER)
void callEXT_SENSORSAPI(String rawUrl, String payload)
{
  rawUrl = EXT_SENSORS_URL + rawUrl;

  Serial.print("new URL:");
  Serial.println(rawUrl);

  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(rawUrl.c_str());

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(payload);

  if (httpResponseCode == 0)
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}
#endif
#if defined(CAMPER) || defined(EXT_SENSORS)
void api_post(AsyncWebServerRequest *request)
{
  String txtResponse = "";
  String rawUrl = request->url();

  Serial.println("Request URL:");
  Serial.println(rawUrl);

  int type = rawUrl.substring(rawUrl.length() - 1).toInt(); // Command = 1, Config = 2

  Serial.print("type:");
  Serial.println(type);

  Serial.println("-----------------------");
  if (type == COMMAND)
  {
    for (size_t i = 0; i < request->params(); i++)
    {
      AsyncWebParameter *param = request->getParam(i);
      int dataEnum = param->name().toInt();
      String dataVal = param->value();

      Serial.printf("Command %u : %s \n", dataEnum, dataVal);

      switch (dataEnum)
      {
      case WINDOW:
#ifdef Servo_pin
        setWindow((dataVal.toInt() == 1));
#else
        callEXT_SENSORSAPI(rawUrl, param->name() + "=" + dataVal);
#endif
        last_WINDOW = (dataVal.toInt() == 1);
        break;
      case RELAY1:

#ifdef Relay1_pin
        setFan((dataVal.toInt() == 1));
#else
        callEXT_SENSORSAPI(rawUrl, param->name() + "=" + dataVal);
#endif
        last_Relay1 = (dataVal.toInt() == 1);
        break;
      case RELAY2:

#ifdef Relay2_pin
        setHeater((dataVal.toInt() == 1));
#else
        callEXT_SENSORSAPI(rawUrl, param->name() + "=" + dataVal);
#endif
        last_Relay2 = (dataVal.toInt() == 1);
        break;
      case DATETIME:
        last_DateTime = dataVal;
        setTime(last_DateTime);
        break;
      }
    }
#if defined(CAMPER)
    // Force a lora send on next loop
    lastLORASend = 0;
#endif
    txtResponse = "Command received";
  }
  if (type == CONFIGS)
  {
    Serial.println("API post Config");
    for (size_t i = 0; i < request->params(); i++)
    {
      AsyncWebParameter *param = request->getParam(i);
      int settingID = param->name().toInt() - 10;
      String dataVal = param->value();

      Serial.printf("Config %s : %s \n", settings[settingID].name, dataVal);

      settings[settingID].value = dataVal.toFloat();

#if defined(CAMPER)
      // Send the config to the Ext Sesor via API
      callEXT_SENSORSAPI(rawUrl, param->name() + "=" + dataVal);
#endif
    }
    savePreferences();

    txtResponse = "Configs saved";
  }

  request->send(200, "text/html", txtResponse);
}
#endif

void setWebHandles()
{
#if defined(CAMPER) || defined(HANDHELD)
  server.serveStatic("/", LittleFS, "/"); // Try the FS first for static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String html = "";
              File file = LittleFS.open("/index.html");
              if (!file)
              {
                html = "";
              }
              else
              {
                html = file.readString();
              }
              file.close();
#if defined(CAMPER)
            String EXT_OTA = "<a href='{IP}/update'><span style='color: red;'>OTA Update EXT SENSORS</span> </a><br>'";
            EXT_OTA.replace("{IP}",EXT_SENSORS_URL);
            html.replace("{EXT_SENSOR_OTA}",EXT_OTA);
#else
            html.replace("{EXT_SENSOR_OTA}","");
#endif
            request->send(200, "text/html", html); });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String html = "";
              File file = LittleFS.open("/config.html");
              if (!file)
              {
                html = "";
              }
              else
              {
                html = file.readString();
              }
              file.close();
#if defined(CAMPER)
              html += "<script>";

              for (size_t i = 0; i < (sizeof(settings) / sizeof(setting)); i++)
              {
                String js = "addParam('{0}','{3}',{1},{2});";
                js.replace("{0}", settings[i].name);
                js.replace("{3}", settings[i].description);
                js.replace("{1}", String(i + CONFIG_SERVO_CLOSED_POS));
                if (settings[i].name == "VDiv_Calib")
                {
                  js.replace("{2}", String(last_Voltage));
                }
                else
                {
                  js.replace("{2}", String(settings[i].value));
                }
                Serial.print("js: ");
                Serial.println(js);
                html += "\n" + js;
              }

              html += "</script>";
#endif
              request->send(200, "text/html", html); });
#endif
#if defined(CAMPER) || defined(EXT_SENSORS)
  // API Endpoints
  server.on("/api/sensors", HTTP_GET, api_get);
  server.on("/api/1", HTTP_POST, api_post);
  server.on("/api/2", HTTP_POST, api_post);
#endif

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "File not found"); });
}
#if defined(CAMPER) || defined(HANDHELD)
class CaptiveRequestHandler : public AsyncWebHandler
{
public:
  CaptiveRequestHandler()
  {
    setWebHandles(); // Routes that are managed and don't need to be redirected
  }
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request)
  {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request)
  {
    Serial.println(F("<- Request redirected to captive portal"));

    AsyncResponseStream *response = request->beginResponseStream("text/plain");
    response->setCode(302);
    response->addHeader(F("Location"), (String)F("http://") + WiFi.softAPIP().toString());
    request->send(response);
  }
};
#endif
void initSite(AsyncWebSocket *webSocket)
{
#if defined(CAMPER) || defined(HANDHELD)
  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }
#endif
  Serial.println(F("Setting handlers"));

  setWebHandles();

#if defined(CAMPER) || defined(HANDHELD)
  // Commands managed via websocket
  server.addHandler(webSocket);
#endif
  Serial.println(F("Start OTA"));
  // Start AsyncElegantOTA
  AsyncElegantOTA.begin(&server);

  // Start webserver
  server.begin();
#if defined(CAMPER) || defined(HANDHELD)
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
#endif
}