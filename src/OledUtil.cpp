#if defined(CAMPER) || defined(HANDHELD)
#include "OledUtil.h"
#if defined(CAMPER)
#include "Sensors.h"
#include "site.h"
#else
#include "LoraUtils.h"
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr;
OneButton *button;

#define CONFIG_ICONS_FONT u8g2_font_waffle_t_all

#define ICON_WIDTH 12
#define ICON_HEIGHT 12
#define ICON_GAP 4
#define ICON_BGAP 2
#define ICON_Y ICON_HEIGHT + ICON_GAP + ICON_BGAP

// Page functions
void drawNothing(){};
void drawHomePage();
void drawTemperaturePage();
void drawHumidityPage();
void drawVoltagePage();
void drawLoraPage();
void drawControlsPage();

// Display Before DeepSleep
void drawDeepSleep();

// Menus
//- Home= display all base infos (temp, hum, volts, RSSI, date, time)
//- Temperature= Big Icon and Text Value
//- Humidity= Big Icon and Text Value
//- Voltage= Big Icon and Text Value
//- LORA Status= Last Mex Millis, RSSI, SNR
//- Servo and Relays= window Open/Closed, Fan on/off, Heater on/off
struct menu_entry_type menu_entry_list[] =
    {
        {CONFIG_ICONS_FONT, 57840 + 0, "Home", drawHomePage},
        {CONFIG_ICONS_FONT, 57408 + 0, "Voltage", drawVoltagePage},
        {CONFIG_ICONS_FONT, 57376 - 3, "Temperature", drawTemperaturePage},
        {CONFIG_ICONS_FONT, 57824 + 4, "Humidity", drawHumidityPage},
        {CONFIG_ICONS_FONT, 58032 + 5, "LORA Status", drawLoraPage},
        {CONFIG_ICONS_FONT, 57504 - 1, "Servo and Relays", drawControlsPage},
        {NULL, 0, NULL, drawNothing}};

struct menu_state current_state = {ICON_BGAP, ICON_BGAP, 0};
struct menu_state destination_state = {ICON_BGAP, ICON_BGAP, 0};

void drawMenu(struct menu_state *state)
{
  // set cursor end line 2 pixel from the bottom
  int y = u8g2->getDisplayHeight();
  y = y - ICON_BGAP;
  u8g2->setCursor(2, y);

  int16_t x;
  uint8_t i;
  x = state->menu_start;
  i = 0;
  while (menu_entry_list[i].icon > 0)
  {
    if (x >= -ICON_WIDTH && x < u8g2->getDisplayWidth())
    {
      u8g2->setFont(menu_entry_list[i].font);
      u8g2->drawGlyph(x, y, menu_entry_list[i].icon);
    }
    i++;
    x += ICON_WIDTH + ICON_GAP;
  }
  // Draw box over current icon (1 px width)
  u8g2->drawFrame(state->frame_position - 1, y - ICON_HEIGHT - 1, ICON_WIDTH + ICON_BGAP, ICON_WIDTH + ICON_BGAP);
};

void to_right(struct menu_state *state)
{
  if (menu_entry_list[state->position + 1].font != NULL)
  {
    if ((int16_t)state->frame_position + 2 * (int16_t)ICON_WIDTH + (int16_t)ICON_BGAP < (int16_t)u8g2->getDisplayWidth())
    {
      state->position++;
      state->frame_position += ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position++;
      state->frame_position = (int16_t)u8g2->getDisplayWidth() - (int16_t)ICON_WIDTH - (int16_t)ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
    }
  }
  else
  {
    // Loop back
    state->position = 0;
    state->frame_position = ICON_BGAP;
    state->menu_start = ICON_BGAP;
  }
}

void to_left(struct menu_state *state)
{
  if (state->position > 0)
  {
    if ((int16_t)state->frame_position >= (int16_t)ICON_BGAP + (int16_t)ICON_WIDTH + (int16_t)ICON_GAP)
    {
      state->position--;
      state->frame_position -= ICON_WIDTH + (int16_t)ICON_GAP;
    }
    else
    {
      state->position--;
      state->frame_position = ICON_BGAP;
      state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
    }
  }
  else
  {
    // Go to last Menu
    int menuLength = (sizeof(menu_entry_list) / sizeof(menu_entry_type)) - 1;
    state->position = menuLength - 1;
    state->frame_position = state->position * (ICON_WIDTH + (int16_t)ICON_GAP);
    state->menu_start = state->frame_position - state->position * ((int16_t)ICON_WIDTH + (int16_t)ICON_GAP);
  }
}

uint8_t towards_int16(int16_t *current, int16_t dest)
{
  if (*current < dest)
  {
    (*current)++;
    return 1;
  }
  else if (*current > dest)
  {
    (*current)--;
    return 1;
  }
  return 0;
}

uint8_t towards(struct menu_state *current, struct menu_state *destination)
{
  uint8_t r = 0;
  r |= towards_int16(&(current->frame_position), destination->frame_position);
  r |= towards_int16(&(current->frame_position), destination->frame_position);
  r |= towards_int16(&(current->menu_start), destination->menu_start);
  r |= towards_int16(&(current->menu_start), destination->menu_start);
  return r;
}

ulong _oledStartMillis;
bool _displayOn;

void turnOffOled()
{
  u8g2->setPowerSave(1); // Turn Off display
  _displayOn = false;
}

void turnOnOled()
{
  u8g2->setPowerSave(0); // Turn ON display
  _oledStartMillis = millis();
  _displayOn = true;
}
uint8_t _flipMode = 1;     // Default 180 Degree flip... because i like the buttons on the right side for the camper
bool _controlMenu = false; // true to navigate and edit the control menu instead of the global menu
uint8_t _controlSelected = 0;
float _int_ext_hand = NAN; // if 1 show external, if 0 shows internal
float _hand_camp = NAN;    // if 1 show HANDHELD Battery, if 0 shows CAMPER

// Button navigation
void click()
{
  Serial.println("click");
  if (_displayOn == false)
  {
    turnOnOled();
  }
  else
  {
    turnOnOled();
    if (!_controlMenu)
    {
      to_right(&destination_state);
    }
    else
    {
      // Move to next control
      _controlSelected++;
      if (_controlSelected > 2)
      {
        _controlSelected = 0;
      }
    }
  }

} // click
void doubleClick()
{
  Serial.println("Double click");
  turnOnOled();

  if (!_controlMenu)
  {
    to_left(&destination_state);
  }
  else
  {
    // change state of current control
    // if sensor, call the appropriate funcition directly
    // if handheld, send command with lora, but change the value locally asap
    String LoRaMessage;
    keys_t currData;
    switch (_controlSelected)
    {
    case 0:
      currData = getDataObj("B_WINDOW");
      if (currData.value == "0")
      {
        currData.value = "1";
      }
      else
      {
        currData.value = "0";
      }
      break;
    case 1:
      currData = getDataObj("B_FAN");
      if (currData.value == "0")
      {
        currData.value = "1";
      }
      else
      {
        currData.value = "0";
      }
      break;
    case 2:
      currData = getDataObj("B_HEATER");
      if (currData.value == "0")
      {
        currData.value = "1";
      }
      else
      {
        currData.value = "0";
      }
      break;
    }

    // Forward Data
#if defined(CAMPER)
    // call EXT_SENSORS API to send the command
    callEXT_SENSORSAPI("api/1", String(currData.id) + "=" + currData.value);
#else
    // send lora command
    LoRaMessage = String(DATA) + "?";
    LoRaMessage += String(currData.id) + "=" + currData.value + "&";
    LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
    // Serial.println(LoRaMessage);
    loraSend(LoRaMessage);
#endif
    // Save data... needed?
    String dummy = getDataVal(currData.key);
    Serial.printf("     currDataVal: %s , from Function: %s \n", currData.value, dummy);
    setDataVal(currData.key, currData.value); //
  }
} // doubleClick
u_long _millisLongPress = 0;
void longPress()
{
  if ((millis() - _millisLongPress) > 2000)
  {
    turnOnOled();
    // Check Current Page
    if (menu_entry_list[destination_state.position].name == "Home")
    {
      if (_flipMode == 1)
      {
        _flipMode = 0;
      }
      else
      {
        _flipMode = 1;
      }
      u8g2->setFlipMode(_flipMode);
      _millisLongPress = millis();
    }
    if (menu_entry_list[destination_state.position].name == "Temperature" ||
        (menu_entry_list[destination_state.position].name == "Humidity"))
    {
      if (isnan(_int_ext_hand))
      {
        _int_ext_hand = 0;
      }
      _int_ext_hand = _int_ext_hand + 1;
#if defined(HANDHELD)
#if !defined(USE_MLX90614)
      if (_int_ext_hand == 3)
      {
        _int_ext_hand = 0;
      }
#else
      if (_int_ext_hand == 4)
      {
        _int_ext_hand = 0;
      }
#endif
#else
      if (_int_ext_hand == 2)
      {
        _int_ext_hand = 0;
      }
#endif

      _millisLongPress = millis();
    }

#if defined(HANDHELD)
    if (menu_entry_list[destination_state.position].name == "Voltage")
    {
      if (_hand_camp == 0)
        _hand_camp = 1;
      else
        _hand_camp = 0;

      _millisLongPress = millis();
    }
#endif

    if (menu_entry_list[destination_state.position].name == "Servo and Relays")
    {
      // Switch to internal menu selection / exit internal menu selection
      //  in this mode:
      //  single click to loop over the 3 internal commands
      //  double click to change ON / OFF
      _controlMenu = !_controlMenu; // change mode
      _controlSelected = 0;         // reset to the first control
      _millisLongPress = millis();
    }
  }
} // longPress

void initOled()
{
#ifdef OLED
  // reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(20);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  delay(20);

  Serial.println("Started OLED");
  // Init display object
  u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, OLED_RST, OLED_SCL, OLED_SDA);

  u8g2->begin();
  u8g2->clearBuffer();
  u8g2->setFlipMode(_flipMode);
  u8g2->setFontMode(1); // Transparent
  u8g2->setDrawColor(1);
  u8g2->setFontDirection(0);

  u8g2->setFont(u8g2_font_unifont_tr);
  u8g2->drawStr(0, (1 * 10) + (2 * (1 - 1)), DEVICE_NAME);
  u8g2->drawStr(0, (2 * 10) + (2 * (2 - 1)), "Starting ...");

  u8g2->sendBuffer();

  delay(2000);
  _oledStartMillis = millis();
  _displayOn = true;

  // Init button
  button = new OneButton(0, true);

  button->attachClick(click);               // right
  button->attachDoubleClick(doubleClick);   // left
  button->attachDuringLongPress(longPress); // current menu Commands

  Serial.println("OLED Finished");
#endif
}

void drawHomePage()
{
  String currVal = "";
  // Icon configs for the current page
  const uint8_t *icon_Font = u8g2_font_open_iconic_all_2x_t;
  int iconH = 16;
  int iconW = 16;

  // Text configs for the current page
  const uint8_t *text_Font = u8g2_font_unifont_tr;
  int textH = 10;

  char buf[256];
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 112 + 11); // CLOCK Font
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("DATETIME");
  // Show time of last message if available
  if (currVal.length() > 0)
  {
    struct tm tm;
    strptime(currVal.c_str(), "%FT%T", &tm);

    strftime(buf, sizeof(buf), "%R", &tm); // HH:MM with seconds in detail page
  }
  else
  {
    // Otherwise millis
    snprintf(buf, sizeof(buf), "%u", last_Millis);
  }

  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 1;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 96); // VOLTAGE Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%s", getDataVal("VOLTS"));
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  // Serial.printf("VOLTS: %s\n", getDataVal("VOLTS"));

  row = 2;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_unifont_t_weather);
  u8g2->drawGlyph(x, y + iconH, 48 + 1); // TEMPERATURE Font Image
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("EXT_TEMP");
  if (currVal != "")
  {
    snprintf(buf, sizeof(buf), "%sC", currVal);
  }
  else
  {
    currVal = getDataVal("TEMP");
    snprintf(buf, sizeof(buf), "%sC", currVal);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  // Serial.printf("TEMP: %sC\n", getDataVal("TEMP"));
  // Serial.printf("EXT_TEMP: %sC\n", getDataVal("EXT_TEMP"));

  row = 2;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_unifont_t_weather);
  u8g2->drawGlyph(x, y + iconH, 48 + 2); // HUMIDITY Font Image
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("EXT_HUM");
  if (currVal != "")
  {
    snprintf(buf, sizeof(buf), "%s%%", currVal);
  }
  else
  {
    currVal = getDataVal("HUM");
    snprintf(buf, sizeof(buf), "%s%%", currVal);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  // Serial.printf("HUM: %sC\n", getDataVal("HUM"));
  // Serial.printf("EXT_HUM: %sC\n", getDataVal("EXT_HUM"));
  row = 3;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 247); // LORA Signal Font Image
  // Text
  u8g2->setFont(text_Font);
#if !defined(HANDHELD)
  snprintf(buf, sizeof(buf), "%.2f,%.2f", last_RSSI, last_SNR);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
#else
  snprintf(buf, sizeof(buf), "%.2f", last_RSSI);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 3;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 96 - 6); // Battery Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%s", getDataVal("HAND_VOLTS"));
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

#endif
}

void drawTemperaturePage()
{
  String currVal = "";
  char buf[256];
  char buf_type[256];
  int iconH = 48;
  int iconW = 48;
  int textH = 16;
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_open_iconic_all_6x_t);
  u8g2->drawGlyph(x, y + iconH, 256 + 3); // SUN Font Image
  // Text
  u8g2->setFont(u8g2_font_inb16_mf);

  if (isnan(_int_ext_hand))
  {
    _int_ext_hand = 0;
  }

  if (_int_ext_hand == 0)
  {
    currVal = getDataVal("EXT_TEMP");
    if (currVal != "")
    {
      snprintf(buf, sizeof(buf), "%s", currVal);
      snprintf(buf_type, sizeof(buf_type), "%s", "c ext");
    }
    else
    {
      _int_ext_hand = 1; // no ext_temp, force to internal
    }
  }
  if (_int_ext_hand == 1)
  {
    currVal = getDataVal("TEMP");
    if (currVal != "")
    {
      _int_ext_hand = _int_ext_hand + 1; // no in_temp, force to next
#if !defined(HANDHELD)
      _int_ext_hand = 0;
#endif
    }
    else
    {
      snprintf(buf, sizeof(buf), "%s", currVal);
      snprintf(buf_type, sizeof(buf_type), "%s", "c int");
    }
  }
#if defined(HANDHELD)
  if (_int_ext_hand == 2)
  {
    snprintf(buf, sizeof(buf), "%s", getDataVal("AMB_TEMP"));
    snprintf(buf_type, sizeof(buf_type), "%s", "h amb");
  }
#if defined(USE_MLX90614)
  if (_int_ext_hand == 3)
  {
    snprintf(buf, sizeof(buf), "%s", getDataVal("IR_TEMP"));
    snprintf(buf_type, sizeof(buf_type), "%s", "h IR");
  }
#endif
#endif

  u8g2->drawStr(x + iconW, y + (textH + 2), buf);
  u8g2->drawStr(x + iconW, y + ((textH + 2) * 2), buf_type);
}

void drawHumidityPage()
{
  String currVal = "";
  char buf[256];
  char buf_type[256];
  int iconH = 48;
  int iconW = 48;
  int textH = 16;
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_open_iconic_all_6x_t);
  u8g2->drawGlyph(x, y + iconH, 160 - 8); // Water Drop Font Image
  // Text
  u8g2->setFont(u8g2_font_inb16_mf);
  if (isnan(_int_ext_hand))
  {
    _int_ext_hand = 0;
  }
  if (_int_ext_hand == 0)
  {
    currVal = getDataVal("EXT_HUM");
    if (currVal != "")
    {
      snprintf(buf, sizeof(buf), "%s", currVal);
      snprintf(buf_type, sizeof(buf_type), "%s", "c ext");
    }
    else
    {
      _int_ext_hand = 1; // no ext_temp, force to internal
    }
  }
  if (_int_ext_hand == 1)
  {
    currVal = getDataVal("HUM");
    if (currVal != "")
    {
      _int_ext_hand = _int_ext_hand + 1; // no in_hum, force to next
#if !defined(HANDHELD)
      _int_ext_hand = 0;
#endif
    }
    else
    {
      snprintf(buf, sizeof(buf), "%s", currVal);
      snprintf(buf_type, sizeof(buf_type), "%s", "c int");
    }
  }
#if defined(HANDHELD)
  if (_int_ext_hand >= 2)
  {
    snprintf(buf, sizeof(buf), "%s", getDataVal("AMB_HUM"));
    snprintf(buf_type, sizeof(buf_type), "%s", "h amb");
  }
#endif

  u8g2->drawStr(x + iconW, y + (textH + 2), buf);
  u8g2->drawStr(x + iconW, y + ((textH + 2) * 2), buf_type);
}

void drawVoltagePage()
{
  char buf[256];
  char buf_type[256];
  int iconH = 48;
  int iconW = 48;
  int textH = 19;
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_open_iconic_all_6x_t);
  if (isnan(_hand_camp))
  {
    _hand_camp = 0;
  }
#if !defined(HANDHELD)
  _hand_camp = 0;
#endif
  if (_hand_camp == 0)
  {
    u8g2->drawGlyph(x, y + iconH, 96 + 0); // BOLT Font Image
  }
  else
  {
    u8g2->drawGlyph(x, y + iconH, 96 - 6); // Battery Font Image
  }
  // Text
  u8g2->setFont(u8g2_font_inb19_mf);

  if (_hand_camp == 0)
  {
    snprintf(buf, sizeof(buf), "%s", getDataVal("VOLTS"));
  }
#if defined(HANDHELD)
  else
  {
    snprintf(buf, sizeof(buf), "%s", getDataVal("HAND_VOLTS"));
  }
#endif

  u8g2->drawStr(x + iconW, y + (textH + 2), buf);

  //  Calculate battPercentage
  uint8_t bp = 0;
  if (_hand_camp == 0)
  {
    float last_Voltage = getDataVal("VOLTS").toFloat();
    for (size_t i = 0; i < (sizeof(batt_perc_12_list) / sizeof(batt_perc)); i++)
    {
      /* table lookup */
      if (last_Voltage >= batt_perc_12_list[i].voltage)
      {
        bp = batt_perc_12_list[i].percentage;
        break;
      }
    }
  }
#if defined(HANDHELD)
  else
  {
    float batt_Voltage = getDataVal("HAND_VOLTS").toFloat();
    for (size_t i = 0; i < (sizeof(batt_perc_3_7_list) / sizeof(batt_perc)); i++)
    {
      /* table lookup */
      if (batt_Voltage >= batt_perc_3_7_list[i].voltage)
      {
        bp = batt_perc_3_7_list[i].percentage;
        break;
      }
    }
  }
#endif
  // Serial.printf("battery perc: %u%%\n", bp);

  snprintf(buf, sizeof(buf), "%u%%", bp);
  u8g2->drawStr(x + iconW, y + ((textH + 2) * 2), buf);
}

void drawLoraPage()
{
  String currVal = "";
  // Icon configs for the current page
  const uint8_t *icon_Font = u8g2_font_streamline_all_t;
  int iconH = 22;
  int iconW = 22;

  // Text configs for the current page
  const uint8_t *text_Font = u8g2_font_lubB14_tr;
  int textH = 14;

  char buf[256];
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 336 + 8); // CLOCK Font
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("DATETIME");
  // Show time of last message if available
  if (currVal.length() > 0)
  {
    struct tm tm;
    strptime(currVal.c_str(), "%FT%T", &tm);

    strftime(buf, sizeof(buf), "%R", &tm); // HH:MM with seconds in detail page
  }
  else
  {
    // Otherwise millis
    snprintf(buf, sizeof(buf), "%u", last_Millis);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 464 - 4); // SATELLITE Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%.2f", last_RSSI);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
}

void drawControlsPage()
{
  String currVal = "";
  // Icon configs for the current page
  const uint8_t *icon_Font = u8g2_font_streamline_all_t;
  int iconH = 22;
  int iconW = 22;

  // Text configs for the current page
  const uint8_t *text_Font = u8g2_font_lubB12_tr;
  int textH = 12;

  char buf[256];
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_streamline_interface_essential_action_t);
  u8g2->drawGlyph(x, y + iconH, 48 + 6); // Arrow Circle Font Image
  if (_controlMenu && _controlSelected == 0)
  {
    u8g2->drawFrame(x, y, iconW + 1, iconH + 1);
  }
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("B_WINDOW");
  if (currVal == "1")
  {
    snprintf(buf, sizeof(buf), "OPEN");
  }
  else
  {
    snprintf(buf, sizeof(buf), "Closed");
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_streamline_ecology_t);
  u8g2->drawGlyph(x, y + iconH, 64 - 1); // WIND Font Image
  if (_controlMenu && _controlSelected == 1)
  {
    u8g2->drawFrame(x, y, iconW + 1, iconH + 1);
  }
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("B_FAN");
  if (currVal == "1")
  {
    snprintf(buf, sizeof(buf), "ON");
  }
  else
  {
    snprintf(buf, sizeof(buf), "OFF");
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_streamline_weather_t);
  u8g2->drawGlyph(x, y + iconH, 48 + 6); // TEMPERATURE Font Image
  if (_controlMenu && _controlSelected == 2)
  {
    u8g2->drawFrame(x, y, iconW + 1, iconH + 1);
  }
  // Text
  u8g2->setFont(text_Font);
  currVal = getDataVal("B_HEATER");
  if (currVal == "1")
  {
    snprintf(buf, sizeof(buf), "ON");
  }
  else
  {
    snprintf(buf, sizeof(buf), "OFF");
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
}

void drawDeepSleep()
{
  int iconH = 21;
  int iconW = 21;
  int textH = 19;
  int row = 0;
  int column = 0;
  int columnW = (u8g2->getDisplayWidth() / 2) - 1;
  int x = 0;
  int y = 0;

  row = 1;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_streamline_interface_essential_loading_t);
  u8g2->drawGlyph(x, y + iconH, 48 + 13); // HourGlass Font Image
  // Text
  u8g2->setFont(u8g2_font_inb16_mf);
  u8g2->drawStr(x + iconW, y + (textH + ((iconH - textH) / 2)), "Deep");

  row = 2;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  u8g2->setFont(u8g2_font_inb16_mf);
  u8g2->drawStr(x + iconW, y + (textH + ((iconH - textH) / 2)), "Sleep...");
}

u_long lastDraw = 0;

void drawPage()
{
  // keep watching the push button
  button->tick();

  if (_displayOn)
  {
    if ((u_long)(millis() - _oledStartMillis) >= (60 * 1000))
    {
      turnOffOled();
    }
    else
    {
      if ((u_long)(millis() - lastDraw) >= 50)
      {
        // Move to the correct menu
        do
        {
        } while (towards(&current_state, &destination_state)); // loop till destination is reached

        // Clear the screen
        u8g2->clearBuffer();

        // Call the page draw function
        (*menu_entry_list[destination_state.position].drawFunc)();

        // Draw bottom menu
        drawMenu(&destination_state);

        // Send pixel to screen
        u8g2->sendBuffer();

        lastDraw = millis();
      }
    }
  }
};
#endif