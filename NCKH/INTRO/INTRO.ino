#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

enum AppMode
{
  MODE_INTRO,
  MODE_MENU,
  MODE_CAL
};

AppMode currentMode = MODE_INTRO;

// ==========================================
void showIntro()
{
  // Background
  tft.fillScreen(0xEFFF);

  // ===== Logo =====
  tft.fillCircle(240, 80, 35, TFT_CYAN);
  tft.drawCircle(240, 80, 35, TFT_BLUE);

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_WHITE, TFT_CYAN);
  tft.drawString("R", 240, 80, 6);

  // ===== Title =====
  tft.setTextColor(TFT_BLACK, 0xEFFF);
  tft.drawString("ReOil", 240, 140, 4);

  tft.setTextColor(TFT_DARKGREY, 0xEFFF);
  tft.drawString("By Khanh & Nguyen", 240, 170, 2);

  // ===== Progress Bar =====
  int x = 70;
  int y = 220;
  int w = 340;
  int h = 22;

  tft.drawRoundRect(
    x,
    y,
    w,
    h,
    8,
    TFT_DARKGREY
  );

  unsigned long startTime = millis();
  int oldProgress = -1;

  while(millis() - startTime < 5000)
  {
    int progress =
      map(
        millis() - startTime,
        0,
        5000,
        0,
        w - 4
      );

    if(progress != oldProgress)
    {
      tft.fillRoundRect(
        x + 2,
        y + 2,
        progress,
        h - 4,
        6,
        TFT_CYAN
      );

      oldProgress = progress;
    }

    tft.setTextColor(
      TFT_NAVY,
      0xEFFF
    );

    tft.drawString(
      "Loading...",
      240,
      260,
      2
    );
  }

  // ===== Transition =====
  tft.fillScreen(TFT_WHITE);
  delay(100);

  tft.fillScreen(0xCFFF);
  delay(100);

  // Chuyển sang MENU
  currentMode = MODE_MENU;
}

// ==========================================
void drawMenu()
{
  tft.fillScreen(TFT_WHITE);

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_BLACK);
  tft.drawString("MENU", 240, 40, 4);

  tft.fillRoundRect(
    140,
    100,
    200,
    70,
    10,
    TFT_CYAN
  );

  tft.drawString(
    "AUTO",
    240,
    135,
    4
  );

  tft.fillRoundRect(
    140,
    200,
    200,
    70,
    10,
    TFT_GREEN
  );

  tft.drawString(
    "ONE TIME",
    240,
    235,
    4
  );
}

// ==========================================
void setup()
{
  tft.init();
  tft.setRotation(1);

  showIntro();

  if(currentMode == MODE_MENU)
  {
    drawMenu();
  }
}

// ==========================================
void loop()
{
}