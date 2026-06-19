#include "HX711.h"
#define PUMPWATER_PIN   2
#define PUMPOIL_PIN 3

#define HX_DT   4
#define HX_SCK  5

void enterProcessing()
{
  currentMode = MODE_PROCESSING;

  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  tft.drawString("PROCESSING", 20, 20, 4);

  tft.drawString(
    "Waste Oil : " + String(oilValue,1) + " g",
    20, 80, 2
  );

  tft.drawString(
    "Water : " + String(waterValue,1) + " g",
    20, 120, 2
  );

  tft.drawString(
    "NaOH : " + String(naohValue,1) + " g",
    20, 160, 2
  );

  // ================= PUMPS =================
  digitalWrite(PUMPWATER_PIN, HIGH);
  digitalWrite(PUMPOIL_PIN, HIGH);

  // ================= STATUS =================
  tft.setTextColor(TFT_GREEN);

  tft.drawString(
    "Pump Water ON",
    20,
    220,
    2
  );

  tft.drawString(
    "Pump Oil ON",
    20,
    245,
    2
  );

  // ================= WEIGHT =================
  tft.setTextColor(TFT_BLUE);

  tft.drawString(
    "Current Weight:",
    250,
    220,
    2
  );

  tft.drawString(
    "0.0 g",
    250,
    245,
    4
  );
}

void processingLoop()
{
  if(millis() - lastWeightUpdate >= 1000)
  {
    lastWeightUpdate = millis();

    currentWeight = scale.get_units(5);

    // xóa vùng số cũ
    tft.fillRect(
      250,
      245,
      180,
      35,
      TFT_WHITE
    );

    // hiện số mới
    tft.setTextColor(TFT_BLUE, TFT_WHITE);

    tft.drawString(
      String(currentWeight,1) + " g",
      250,
      245,
      4
    );

    Serial.print("Weight: ");
    Serial.println(currentWeight);
  }
}