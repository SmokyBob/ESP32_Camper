#include "OledUtil.h"

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

// Menus
//- Home= display all base infos (temp, hum, volts, RSSI, date, time)
//- Temperature= Big Icon and Text Value
//- Humidity= Big Icon and Text Value
//- Voltage= Big Icon and Text Value
//- LORA Status= Last Mex ID, RSSI, SNC
//- LORA configs= Freq, Bandwidth, RX
struct menu_entry_type menu_entry_list[] =
    {
        {CONFIG_ICONS_FONT, 57840, "Home", drawHomePage},
        {CONFIG_ICONS_FONT, 57373, "Temperature", drawTemperaturePage},
        {CONFIG_ICONS_FONT, 57828, "Humidity", drawNothing},
        {CONFIG_ICONS_FONT, 57857, "Voltage", drawNothing},
        {CONFIG_ICONS_FONT, 58037, "LORA Status", drawNothing},
        {CONFIG_ICONS_FONT, 58065, "LORA Configs", drawNothing},
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

// Button navigation
void click()
{
  Serial.println("click");
  to_right(&destination_state);
} // click
void doubleClick()
{
  Serial.println("Double click");
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
  u8g2->drawGlyph(x, y + iconH, 184); // TODO: find timer / hourglass Icon
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
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 152); // TODO: find temperature Icon (no sun .. maybe other 16x16 font? ...or use the images )
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%.0f C", last_Temperature);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);

  row = 2;
  column = 2;
  x = (column - 1) * columnW;
  y = (row - 1) * iconH;
  // Icon
  u8g2->setFont(icon_Font);
  u8g2->drawGlyph(x, y + iconH, 152); // HUMIDITY Font Image
  // Text
  u8g2->setFont(text_Font);
  snprintf(buf, sizeof(buf), "%.2f rh", last_Humidity);
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
  int row = 1;
  int column = 1;
  int columnW = (128 / 2) - 1;
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
  snprintf(buf, sizeof(buf), "%.0f C", last_Temperature);
  u8g2->drawStr(x + iconW + ICON_BGAP, y + (textH + ((iconH - textH) / 2)), buf);
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

    //Send pixel to screen
    u8g2->sendBuffer();

    lastDraw = millis();
  }
};