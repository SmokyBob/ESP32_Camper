#ifdef BLE_APP
#include "RemoteXYUtils.h"

void BLE_APP_setup() // Renamed from setup, called in main.cpp setup
{
  RemoteXY_Init();

  // TODO: set statues from settings
}

void BLE_APP_loop()
{
  RemoteXY_Handler();

  // TODO you loop code
  // use the RemoteXY structure for data transfer
  // do not call delay()

  if (RemoteXY.connect_flag == 1)
  {
  }
}
#endif