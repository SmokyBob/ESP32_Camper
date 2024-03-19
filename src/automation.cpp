#include "automation.h"

void runAutomation()
{
  // TODO: clone data[] in automationData[]
  //     loop over all automation configurations IN ORDER
  //     evaluate only enabled=true
  //     evaluate action and Update automationData[action_id] if condition is true
  //     after all automation is evaluated
  //     loop over automationData[] and compare with data[]
  //       if different, update module status (ex window open/close or heater on/off or fan on/off)
  float currTemp = -1000; // Start with extreme negative value to manage nan value / no temp sensor working
  String currVal = "";
#if defined(EXT_DHT22_pin) || defined(EXT_SHT2_SDA)
  currVal = getDataVal("EXT_TEMP");
  if (currVal != "")
  {
    currTemp = currVal.toFloat();
  }
#else
  currVal = getDataVal("TEMP");
  if (currVal != "")
  {
    currTemp = currVal.toFloat();
  }

#endif

#ifdef Servo_pin
  if (currTemp > -1000)
  {
    currVal = getDataVal("B_WINDOW");
    // From settings
    if (currTemp >= getConfigVal("SERVO_OP_TEMP").toFloat()) // default 30
    {
      if (currVal.toInt() == 0)
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

    if (currTemp <= getConfigVal("SERVO_CL_TEMP").toFloat()) // default 20
    {
      if (currVal.toInt() == 1)
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
    currVal = getDataVal("B_HEATER");
    // From settings
    if (currTemp >= getConfigVal("HEAT_OFF_TEMP").toFloat()) // default 18
    {
      if (currVal.toInt() == 1)
      {
        // Turn heater OFF
        setHeater(false);
        setFan(false);
      }
    }
    if (currTemp <= getConfigVal("HEAT_ON_TEMP").toFloat()) // default 15
    {
      if (currVal.toInt() == 0)
      {
        // Turn heater on
        setHeater(true);
      }
    }
  }
#endif
}