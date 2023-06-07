#include "OledUtil.h"
#ifdef SENSORS
#include "Sensors.h"
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
        {CONFIG_ICONS_FONT, 57376 - 3, "Temperature", drawTemperaturePage},
        {CONFIG_ICONS_FONT, 57824 + 4, "Humidity", drawHumidityPage},
        {CONFIG_ICONS_FONT, 57408 + 0, "Voltage", drawVoltagePage},
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

uint8_t _flipMode = 1;     // Default 180 Degree flip... because i like the buttons on the right side
bool _controlMenu = false; // true to navigate and edit the control menu instead of the global menu
uint8_t _controlSelected = 0;
float _int_ext = NAN; // if 1 show external, if 0 shows internal
// Button navigation
void click()
{
  Serial.println("click");
  if (!_controlMenu)
  {
    to_right(&destination_state);
  }
  else
  {
    // TODO: highlight next control
    _controlSelected++;
    if (_controlSelected > 2)
    {
      _controlSelected = 0;
    }
  }

} // click
void doubleClick()
{
  Serial.println("Double click");

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
    switch (_controlSelected)
    {
    case 0:
#ifdef SENSORS
      setWindow(!last_WINDOW);
#else
      // send lora command
      LoRaMessage = String(COMMAND) + "?";
      LoRaMessage += String(WINDOW) + "=" + String(last_WINDOW) + "&";
      LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
      // Serial.println(LoRaMessage);
      loraSend(LoRaMessage);
#endif
      break;
    case 1:
      last_Relay1 = !last_Relay1;
#ifdef SENSORS
      setFan(last_Relay1);
#else

      // send lora command
      LoRaMessage = String(COMMAND) + "?";
      LoRaMessage += String(RELAY1) + "=" + String(last_Relay1) + "&";
      LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
      // Serial.println(LoRaMessage);
      loraSend(LoRaMessage);
#endif
      break;
    case 2:
      last_Relay2 = !last_Relay2;
#ifdef SENSORS
      setHeater(last_Relay2);
#else
      // send lora command
      LoRaMessage = String(COMMAND) + "?";
      LoRaMessage += String(RELAY2) + "=" + String(last_WINDOW) + "&";
      LoRaMessage = LoRaMessage.substring(0, LoRaMessage.length() - 1);
      // Serial.println(LoRaMessage);
      loraSend(LoRaMessage);
#endif
      break;
    }
  }
} // doubleClick
u_long _millisLongPress = 0;
void longPress()
{
  if ((millis() - _millisLongPress) > 2000)
  {
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
      if (_int_ext == 0)
        _int_ext = 1;
      else
        _int_ext = 0;

      _millisLongPress = millis();
    }

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

  delay(3000);

  // Init button
  button = new OneButton(0, true);

  button->attachClick(click);               // right
  button->attachDoubleClick(doubleClick);   // left
  button->attachDuringLongPress(longPress); // current menu Commands

#endif
}

void drawHomePage()
{
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
  snprintf(buf, sizeof(buf), "%u", last_Millis); // TODO: maybe last time? no day
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
  snprintf(buf, sizeof(buf), "%.2f", last_Voltage);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_unifont_t_weather);
  u8g2->drawGlyph(x, y + iconH, 48 + 1); // TEMPERATURE Font Image
  // Text
  u8g2->setFont(text_Font);
  if (isnan(last_Ext_Temperature) != true)
  {
    snprintf(buf, sizeof(buf), "%.2fC", last_Ext_Temperature);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%.0f C", last_Temperature);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(u8g2_font_unifont_t_weather);
  u8g2->drawGlyph(x, y + iconH, 48 + 2); // HUMIDITY Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%.0f%%", last_Humidity);
  if (isnan(last_Ext_Humidity) != true)
  {
    snprintf(buf, sizeof(buf), "%.2f%%", last_Ext_Humidity);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%.0f%%", last_Humidity);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  // TODO: maybe drop this for window status?
  row = 3;
  column = 1;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 247); // LORA Signal Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%.2f,%.2f", last_RSSI, last_SNR);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
}

void drawTemperaturePage()
{
  char buf[256];
  int iconH = 48;
  int iconW = 48;
  int textH = 46;
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
  u8g2->setFont(u8g2_font_inb46_mf);

  if (isnan(_int_ext))
  {
    _int_ext = 0;
  }
  if (_int_ext == 0)
  {
    if (isnan(last_Ext_Temperature) != true)
    {
      snprintf(buf, sizeof(buf), "%.2fC", last_Ext_Temperature);
    }
    else
    {
      snprintf(buf, sizeof(buf), "%.0fC", last_Temperature);
    }
  }
  else
  {
    snprintf(buf, sizeof(buf), "%.0fC", last_Temperature);
  }

  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
}

void drawHumidityPage()
{
  char buf[256];
  int iconH = 48;
  int iconW = 48;
  int textH = 46;
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
  u8g2->setFont(u8g2_font_inb46_mf);
  if (isnan(_int_ext))
  {
    _int_ext = 0;
  }
  if (_int_ext == 0)
  {
    if (isnan(last_Ext_Humidity) != true)
    {
      snprintf(buf, sizeof(buf), "%.2f%%", last_Ext_Humidity);
    }
    else
    {
      snprintf(buf, sizeof(buf), "%.0f%%", last_Humidity);
    }
  }
  else
  {
    snprintf(buf, sizeof(buf), "%.0f%%", last_Humidity);
  }
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
}

void drawVoltagePage()
{
  char buf[256];
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
  u8g2->drawGlyph(x, y + iconH, 96 + 0); // BOLT Font Image
  // Text
  u8g2->setFont(u8g2_font_inb19_mf);
  snprintf(buf, sizeof(buf), "%.2f", last_Voltage);
  u8g2->drawStr(x + iconW, y + (textH + 2), buf);

  // Calculate battPercentage
  uint8_t bp = 0;

  for (size_t i = 0; i < (sizeof(batt_perc_list) / sizeof(batt_perc)); i++)
  {
    /* table lookup */
    if (last_Voltage >= batt_perc_list[i].voltage)
    {
      bp = batt_perc_list[i].percentage;
      break;
    }
  }
  // Serial.printf("battery perc: %u%%\n", bp);

  snprintf(buf, sizeof(buf), "%u%%", bp);
  u8g2->drawStr(x + iconW, y + ((textH + 2) * 2), buf);
}

void drawLoraPage()
{
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
  snprintf(buf, sizeof(buf), "%u", last_Millis);
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
  if (last_WINDOW)
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
  if (last_Relay1)
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
  if (last_Relay2)
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
};