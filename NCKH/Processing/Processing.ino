#define PUMP_PIN 2

void enterProcessing()
{
  currentMode = MODE_PROCESSING;

  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);

  tft.drawString("PROCESSING",20,20,4);

  tft.drawString(
    "Waste Oil : " + String(oilValue,1) + " g",
    20,80,2
  );

  tft.drawString(
    "Water : " + String(waterValue,1) + " g",
    20,120,2
  );

  tft.drawString(
    "NaOH : " + String(naohValue,1) + " g",
    20,160,2
  );

  digitalWrite(PUMP_PIN,HIGH);

  tft.setTextColor(TFT_GREEN);
  tft.drawString("Pump ON",20,220,4);
}