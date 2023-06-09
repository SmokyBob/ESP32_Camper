#include "site.h"
#include "globals.h"
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);

void setWebHandles()
{
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
#ifdef SENSORS
              html += "<script>";

              for (size_t i = 0; i < (sizeof(settings) / sizeof(setting)); i++)
              {
                String js = "addParam('{0}',{1},{2});";
                js.replace("{0}", settings[i].name);
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

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "File not found"); });
}

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

void initSite(AsyncWebSocket *webSocket)
{
  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  Serial.println(F("Setting handlers"));

  setWebHandles();

  // Commands managed via websocket
  server.addHandler(webSocket);
  // Start AsyncElegantOTA
  AsyncElegantOTA.begin(&server);
  // Start webserver
  server.begin();

  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); // only when requested from AP
}