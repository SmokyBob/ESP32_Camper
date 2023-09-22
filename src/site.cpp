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

  request->send(200, "application/json", jsonString);
}

void api_post(AsyncWebServerRequest *request)
{
  String jsonResponse = "";

  int type = request->pathArg(0).toInt(); // Command = 1, Config = 2

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
    }
#if defined(CAMPER)
    // Force a lora send on next loop
    lastLORASend = 0;
#endif
    jsonResponse = "Command received";
  }
  if (type == CONFIGS)
  {
    Serial.println("API post Config");
    for (size_t i = 0; i < request->params(); i++)
    {
      AsyncWebParameter *param = request->getParam(i);
      int settingID = 10 - param->name().toInt();
      String dataVal = param->value();

      Serial.printf("Config %s : %s \n", settings[settingID].name, dataVal);

      settings[settingID].value = dataVal.toFloat();
    }
    savePreferences();

    jsonResponse = "Configs saved";
  }

  request->send(200, "text/html", jsonResponse);
}

void setWebHandles()
{
#if defined(CAMPER) || defined(HANDHELD)
  server.serveStatic("/", LittleFS, "/"); // Try the FS first for static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });

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
  server.on("^\\/api\\/([0-9]+)$", HTTP_POST, api_post);
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