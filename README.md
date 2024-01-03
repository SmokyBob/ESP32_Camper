# ABOUT
Off-roading is fun and let you connect with nature, but we are in 2023 and we like our luxuries... so a battery for the fridge, an heater, a fan, an inverter...

This project provides basic monitoring of a 12v services battery, temperature and humidity; heater, fan and vent control.

This Project is mostly tailored to my needs but can be forked

# FEATURES
- 1 code for 3 devices
  - Different build configurations set variabiles and load libraries as needed
- Web Access
  - For monitoring, commands, configuration and OTA Updates
- Long range access via LORA
  - For monitoring and command
- BLE monitoring and control with notifications
  - For monitoring, commands and configuration
  - UI via Offline Web App at https://smokybob.github.io/ESP32_Camper/
- Automate tasks
  - Open / Close vent at specific temperatures
  - Turn fan on / off at specific temperatures
  - Turn heater on / off at specific temperatures

# DEVICES
## CAMPER_OLED
*TODO photos and schematics* 
## EXT_SENSORS
*TODO photos and schematics* 
## HANDELD_OLED
*TODO photos and schematics* 

# GETTING STARTED
## Compiling and Flashing
*TODO*
## Usage
*TODO*

# TODOS
## Code
- [ ] BLE UI, Tap Friendly sliders on mobile
- [ ] BLE UI, Configuration edit
- [ ] BLE UI, Configurable notifications saved in local storage on the UI device
- [ ] Custom "camper" automations configurable in a list
- [ ] Simple Logging 
  - last 10 "Events" stored on the device
  - Ex. 
    - 2023-12-06 18:17:05 Automation 'heater ON temp low' triggered
    - 2023-12-06 18:14:00 Automation 'window Close temp low' triggered
- [ ] Data Optimization (es. no last_X variables, but an array of object with map to the Enumerators)
- [ ] Async sensors reading in dedicated threads

## Hardware
- [x] Better Handheld battery and charging
- [x] High power MOSFET, the board is cheap and easy but limited to 15A ... and for the heater a bigger mosfet with a heatsync would be better then the hacked up piece of aluminium currently in use
  - a simpler way was to use an automotive 30a 12v Relay, triggered with the mosfet board (this way the high amps are managed by the relay)
- [ ] CAMPER_OLED temperature sensor, because it's near the battery and we could set heater automation profiles to keep the battery in a good temperature range for charging
- [ ] SDCard expansion for extensive logging

# DISLAIMER
The code within this repository comes with no guarantee, use it on your own risk.

Don't touch these firmwares if you don't know how to put the device in the programming mode if something goes wrong. As per the GPL v3 license, I assume no liability for any damage to you or any other person or equipment.