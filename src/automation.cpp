#include "automation.h"
#include "Sensors.h"

void runAutomation()
{
  float currTemp = -1000;//Start with extreme negative value to manage nan value / no temp sensor working

#ifdef EXT_DHT22_pin
  if (isnan(last_Ext_Temperature) != true)
  {
    currTemp = last_Ext_Temperature;
  }
#else
  currTemp = last_Temperature;
#endif

  // TODO: automation with manual ovverride
#ifdef Servo_pin
  if (currTemp > -1000)
  {
    // From settings
    if (currTemp >= settings[3].value) // default 30
    {
      if (!last_WINDOW)
      {

        // webSocket->textAll("LastWindow: " + String(last_WINDOW) + " set to Open = 1");
        // Open the window
        setWindow(true);
      }
    }

    if (currTemp <= settings[2].value) // default 20
    {
      if (last_WINDOW)
      {
        // webSocket->textAll("LastWindow: " + String(last_WINDOW) + " set to Closed = 0");
        // Close the window
        setWindow(false);
      }
    }
  }
#endif
  // TODO: automation for FAN and HEATER
}