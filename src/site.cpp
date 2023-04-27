#include "site.h"
// TODO: AsyncWebServer and AsyncOTA
// TODO: WebSocket 2 ways communication

AsyncWebServer server(80);

void initSite(AsyncWebSocket webSocket)
{
  if (!LittleFS.begin(true))
  {
    Serial.println(F("An Error has occurred while mounting LittleFS"));
    return;
  }

  Serial.println(F("Setting handlers"));
  server.serveStatic("/", LittleFS, "/"); // Try the FS first for static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false, nullptr); });

  // TODO: Config page
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(200, "text/plain", "File not found"); });

  //Commands managed via websocket
  server.addHandler(&webSocket);
  server.begin();
}
