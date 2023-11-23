#include "site.h"
#include "globals.h"
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);
#if defined(CAMPER) || defined(EXT_SENSORS)
void api_get(AsyncWebServerRequest *request)
{
  Serial.print("api URL:");
  Serial.println(request->url());
  Serial.println("");

  String rawUrl = request->url();

  if (request->url().indexOf("api/sensors") > 0)
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
#if defined(CAMPER)
    jsonString += "\"automation\":\"" + String((int)settings[8].value) + "\",";
    String tmpBool = "0";
    if (last_IgnoreLowVolt != "")
    {
      tmpBool = "1";
    }

    jsonString += "\"220power\":\"" + String(tmpBool) + "\",";

#endif

    jsonString += "\"dummy\":null}";

    Serial.print("api get: ");
    Serial.println(jsonString);

    request->send(200, "application/json", jsonString);
  }
  if (request->url().startsWith("/api/1"))
  {
    // Command
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
      case IGNORE_LOW_VOLT:
        if (dataVal.toInt() == 1)
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

        Serial.print("api\\get   last_IgnoreLowVolt: ");
        Serial.println(last_IgnoreLowVolt);
        break;
      }
    }
#if defined(CAMPER)
    // Force a lora send on next loop
    lastLORASend = 0;
#endif
    request->send(200, "text/html", "Command received");
  }
  if (request->url().startsWith("/api/2"))
  {
    // CONFIG
    Serial.println("API get Config");
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
    request->send(200, "text/html", "Command received");
  }
}
#endif
#if defined(CAMPER)
void callEXT_SENSORSAPI(String rawUrl, String payload)
{

  String ReqUrl = EXT_SENSORS_URL + "/" + rawUrl + "?" + payload;

  Serial.print("ReqUrl:");
  Serial.println(ReqUrl);

  String toRet = "";
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(ReqUrl.c_str());

  // Send HTTP GET request
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    String payload = http.getString();

    Serial.print("Response: ");
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

#endif

void setWebHandles()
{
#if defined(CAMPER) || defined(EXT_SENSORS)
  // API Endpoints
  server.on("/api/sensors", HTTP_GET, api_get);
  server.on("/api/1", HTTP_GET, api_get);
  server.on("/api/2", HTTP_GET, api_get);
#endif
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
            String EXT_OTA = "<a href='{IP}/update'><span style='color: red;'>OTA Update EXT SENSORS</span> </a><br>";
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