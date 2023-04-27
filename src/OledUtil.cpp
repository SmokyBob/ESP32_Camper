#include "OledUtil.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr;
OneButton *button;

// Menus
//- Home= display all base infos (temp, hum, volts, RSSI, date, time)
//- Temperature= Big Icon and Text Value
//- Humidity= Big Icon and Text Value
//- Voltage= Big Icon and Text Value
//- LORA Status= Last Mex ID, RSSI, SNC
//- LORA configs= Freq, Bandwidth, RX
struct menu_entry_type menu_entry_list[] =
    {
        {CONFIG_ICONS_FONT, 184, "Home"},
        {CONFIG_ICONS_FONT, 129, "Temperature"},
        {CONFIG_ICONS_FONT, 152, "Humidity"},
        {CONFIG_ICONS_FONT, 96, "Voltage"},
        {CONFIG_ICONS_FONT, 247, "LORA Status"},
        {CONFIG_ICONS_FONT, 253, "LORA Configs"},
        {NULL, 0, NULL}};

struct menu_state current_state = {ICON_BGAP, ICON_BGAP, 0};
struct menu_state destination_state = {ICON_BGAP, ICON_BGAP, 0};

void clearArea(int x, int y, int width, int height)
{
  // clear area (draw with background color)
  u8g2->setDrawColor(0);
  u8g2->drawBox(x, y, width, height);
  u8g2->setDrawColor(1);
}

void drawMenu(struct menu_state *state)
{
  // int structLen = (sizeof(menu_entry_list) / sizeof(menu_entry_type)) - 1;
  // set cursor end line 2 pixel from the bottom
  u8g2->setCursor(2, u8g2->getDisplayHeight() - ICON_HEIGHT - 2);

  int16_t x;
  uint8_t i;
  x = state->menu_start;
  i = 0;
  while (menu_entry_list[i].icon > 0)
  {
    if (x >= -ICON_WIDTH && x < u8g2->getDisplayWidth())
    {
      u8g2->setFont(menu_entry_list[i].font);
      u8g2->drawGlyph(x, ICON_Y, menu_entry_list[i].icon);
    }
    i++;
    x += ICON_WIDTH + ICON_GAP;
  }
  u8g2->drawFrame(state->frame_position - 1, ICON_Y - ICON_HEIGHT - 1, ICON_WIDTH + 2, ICON_WIDTH + 2);
  u8g2->drawFrame(state->frame_position - 2, ICON_Y - ICON_HEIGHT - 2, ICON_WIDTH + 4, ICON_WIDTH + 4);
  u8g2->drawFrame(state->frame_position - 3, ICON_Y - ICON_HEIGHT - 3, ICON_WIDTH + 6, ICON_WIDTH + 6);
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

// Button navigation
void click()
{
  to_right(&destination_state);
} // click
void doubleClick()
{
  to_left(&destination_state);
} // doubleClick
void longPress()
{
  // TODO: turn off the display / go to sleep
} // longPress

void initOled()
{
#ifdef OLED
  Wire.begin(OLED_SDA, OLED_SCL);

  // reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(20);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  delay(20);

  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
  {
    Serial.println("Started OLED");
    // Init display object
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);

    u8g2->begin();
    u8g2->clearBuffer();
    u8g2->setFlipMode(1); // 180 Degree flip... because i like the buttons on the right side
    u8g2->setFontMode(1); // Transparent
    u8g2->setDrawColor(1);
    u8g2->setFontDirection(0);

    u8g2->setFont(u8g2_font_unifont_tr);
    u8g2->drawStr(0, (1 * 10) + (2 * (1 - 1)), DEVICE_NAME);
    u8g2->drawStr(0, (2 * 10) + (2 * (2 - 1)), "Starting ...");

    u8g2->sendBuffer();

    delay(3000);
  }

  // Init button
  button = new OneButton(0, true);

  button->attachClick(click);               // right
  button->attachDoubleClick(doubleClick);   // left
  button->attachDuringLongPress(longPress); // turnScreenOFF / Sleep

#endif
}

void drawHomePage()
{
}

void drawPage()
{
  u8g2->clearBuffer();

  //TODO: Other menus and specific pages
  if (menu_entry_list[current_state.position].name == "HOME")
  {
    drawHomePage();
  }
  drawMenu(&current_state); // Draw bottom menu
};