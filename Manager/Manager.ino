#include <SPI.h>
#include <XPT2046_Bitbang.h>
#include <TFT_eSPI.h>
#include "storage.h"
#include "FS.h"
#include "SD.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define KB_ROWS 3
#define KB_COLUMNS 5

XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS);

TFT_eSPI tft = TFT_eSPI();

TFT_eSPI_Button key[KB_COLUMNS * KB_ROWS];
char kb[KB_ROWS][KB_COLUMNS] = {{'0', '1', '2', '3', '4'}, {'5', '6', '7', '8', '9'}, {'!', '@', '#', '%', '<'}};
String insertedPassword = "";

void setup()
{
  Serial.begin(115200);

  // Display init
  ts.begin();
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeMono18pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(10, 200, 1);
  tft.print("None");
  drawButtons();

  // FS init
  SPIClass spi = SPIClass(VSPI);
  if (!SD.begin(SS, spi, 80000000)) {
    Serial.println("[FATAL] Card Mount Failed");
    tft.print("SD error");
    while (1) {} // Hang on error...
  }

}

void drawButtons()
{
  uint16_t bWidth = 60;
  uint16_t bHeight = TFT_WIDTH / 4;
  int btnGlobalIndex = 0;
  for (int row = 0; row < KB_ROWS; row++)
  {
    for (int btn = 0; btn < KB_COLUMNS; btn++)
    {
      key[btnGlobalIndex].initButton(&tft,
                                     bWidth * btn + bWidth / 2,
                                     bHeight * row + bHeight / 2 + 20,
                                     bWidth,
                                     bHeight,
                                     TFT_BLACK, // Outline
                                     TFT_RED,   // Fill
                                     TFT_BLACK, // Text
                                     "",
                                     2);
      key[btnGlobalIndex].drawButton(false, String(kb[row][btn]));
      btnGlobalIndex++;
    }
  }
}

void loop()
{
  TouchPoint p = ts.getTouch();
  for (uint8_t b = 0; b < KB_ROWS * KB_COLUMNS; b++)
  {
    if ((p.zRaw > 0) && key[b].contains(p.x, p.y))
    {
      key[b].press(true);
    }
    else
    {
      key[b].press(false);
    }
  }
  for (uint8_t b = 0; b < KB_ROWS * KB_COLUMNS; b++)
  {
    char btnSymb = kb[b / KB_COLUMNS][b - b / KB_COLUMNS * KB_COLUMNS];
    if (key[b].justPressed())
    {
      if (btnSymb == '<')
        insertedPassword = insertedPassword.substring(0, insertedPassword.length() - 1);
      else
        insertedPassword += btnSymb;

      Serial.printf("%s\n", insertedPassword.c_str());
      key[b].drawButton(true, String(btnSymb));

      tft.setCursor(10, 200, 1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.print("                  ");
      tft.setCursor(10, 200, 1);
      for (int i = 0; i < insertedPassword.length(); i++)
        tft.print("*");
    }
    if (key[b].justReleased())
    {
      key[b].drawButton(false, String(btnSymb));
    }
  }
  delay(50);
}
