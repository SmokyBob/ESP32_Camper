#include "automation.h"

void runAutomation()
{
  float currTemp = -1000; // Start with extreme negative value to manage nan value / no temp sensor working

#if defined(EXT_DHT22_pin) || defined(EXT_SHT2_SDA)
  if (isnan(last_Ext_Temperature) != true)
  {
    currTemp = last_Ext_Temperature;
  }
#else
  currTemp = last_Temperature;
#endif

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
#ifdef Relay1_pin
        // Start the FAN
        setFan(true);
#endif
      }
    }

    if (currTemp <= settings[2].value) // default 20
    {
      if (last_WINDOW)
      {
        // webSocket->textAll("LastWindow: " + String(last_WINDOW) + " set to Closed = 0");
        // Close the window
        setWindow(false);
#ifdef Relay1_pin
        // Stop the FAN
        setFan(false);
#endif
      }
    }
  }
#endif

#ifdef Relay2_pin
  if (currTemp > -1000)
  {
    // From settings
    if (currTemp >= settings[10].value) // default 18
    {
      if (last_Relay2){
        //Turn heater OFF
        setHeater(false);
        setFan(false);
      }
    }
    if (currTemp <= settings[9].value) // default 15
    {
      if (!last_Relay2){
        //Turn heater on
        setHeater(true);
      }
    }
  }
#endif
}