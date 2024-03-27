#include "site.h"
#include "globals.h"
#include <ElegantOTA.h>

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

    // Serial.print("api get: ");
    // Serial.println(jsonString);

    request->send(200, "application/json", jsonString);
  }
  if (request->url().startsWith("/api/1"))
  {
    // Command/data
    for (size_t i = 0; i < request->params(); i++)
    {
      AsyncWebParameter *param = request->getParam(i);
      int dataEnum = param->name().toInt();
      String dataVal = param->value();

      Serial.printf("Command %u : %s \n", dataEnum, dataVal);
      bool bFound = false;
      // Loop over the data array
      for (size_t i = 0; i < (sizeof(data) / sizeof(keys_t)); i++)
      {
        // Same id, update value
        if (data[i].id == dataEnum)
        {
          data[i].value = dataVal;
          bFound = true;

          // data with commands
          if (strcmp(data[i].key, "B_WINDOW") == 0)
          {
#ifdef Servo_pin
            setWindow((dataVal == "1"));
#endif
#if defined(CAMPER)
            // call EXT_SENSORS API to send the command
            callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
#endif
          }
          if (strcmp(data[i].key, "B_FAN") == 0)
          {
#ifdef Relay1_pin
            setFan((dataVal == "1"));
#endif
#if defined(CAMPER)
            // call EXT_SENSORS API to send the command
            callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
#endif
          }
          if (strcmp(data[i].key, "B_HEATER") == 0)
          {
#ifdef Relay2_pin
            setHeater((dataVal == "1"));
#endif
#if defined(CAMPER)
            // call EXT_SENSORS API to send the command
            callEXT_SENSORSAPI("api/1", String(data[i].id) + "=" + dataVal);
#endif
            if (strcmp(data[i].key, "DATETIME") == 0)
            {
              setDateTime(dataVal);
            }
          }

          break; // found, exit loop
        }
      }
      if (!bFound)
      {
        // check in configs
        for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
        {
          // Same id, update value
          if (config[i].id == dataEnum)
          {
            config[i].value = dataVal;
            bFound = true;
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
            break;
          }
        }
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
      int dataEnum = param->name().toInt();
      String dataVal = param->value();

      Serial.printf("Config %u : %s \n", dataEnum, dataVal);
      bool bFound = false;
      // check in configs
        for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
        {
          // Same id, update value
          if (config[i].id == dataEnum)
          {
            config[i].value = dataVal;
            bFound = true;
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
            break;
          }
        }
      

#if defined(CAMPER)
      // Send the config to the Ext Sesor via API
      callEXT_SENSORSAPI(rawUrl, param->name() + "=" + dataVal);
#endif
    }
    savePreferences();
    request->send(200, "text/html", "Command received");
  }
  if (request->url().startsWith("/api/3"))
  {
    // Automation
    // TODO: Save automation conditions
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
  server.on("/api/1", HTTP_GET, api_get); // Command/data
  server.on("/api/2", HTTP_GET, api_get); // Configs
  server.on("/api/3", HTTP_GET, api_get); // Automation
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

              for (size_t i = 0; i < (sizeof(config) / sizeof(keys_t)); i++)
              {
                String js = "addParam('{0}','{3}',{1},{2});";
                js.replace("{0}", config[i].key);
                js.replace("{3}", config[i].description);
                js.replace("{1}", String(config[i].id));
                if (strcmp(config[i].key,"VOLT_ACTUAL")==0)
                {
                  js.replace("{2}", getDataVal("VOLTS"));
                }
                else
                {
                  js.replace("{2}", String(config[i].value));
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
// TODO: automation page similar to config with dinamic rows and postback values
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
  // Start ElegantOTA (async using build flag)
  ElegantOTA.begin(&server);

  // Start webserver
  server.begin();
#if defined(CAMPER) || defined(HANDHELD)
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
#endif
}