#include "OledUtil.h"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr;

// TODO: Menus
//- Home= display all base infos (temp, hum, volts, RSSI, date, time)
//- Temperature= Big Icon and Text Value
//- Humidity= Big Icon and Text Value
//- Voltage= Big Icon and Text Value
//- LORA Status= Last Mex ID, RSSI, SNC
//- LORA configs= Freq, Bandwidth, RX
struct menu_entry_type menu_entry_list[] =
    {
        {CONFIG_ICONS_FONT, 184, "Home"},
        {CONFIG_ICONS_FONT, 72, "Configuration"},
        {NULL, 0, NULL}};

struct menu_state current_state = {ICON_BGAP, ICON_BGAP, 0};
struct menu_state destination_state = {ICON_BGAP, ICON_BGAP, 0};

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

#endif
}

void clearArea(int x, int y, int width, int height)
{
  // clear area (draw with background color)
  u8g2->setDrawColor(0);
  u8g2->drawBox(x, y, width, height);
  u8g2->setDrawColor(1);
}

void drawMenu(){
    // TODO: Menu
};
