//=====PostProcessing.ino file=====

void enterPostProcessing() {
  playModeMusic(5);

  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.drawString("POST PROCESSING", 240, 30, 4);

  gateServo.write(90);
  delay(5000);  //change when make prototype (15000)
  liftServo.write(90);
  delay(5000);
}

void postProcessingLoop() {
  static bool firstRun = true;

  if (firstRun) {
    firstRun = false;
    enterPostProcessing();
  }

  // code xử lý sau này
}