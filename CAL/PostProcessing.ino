//=====PostProcessing.ino file=====

void enterPostProcessing() {
  playModeMusic(5);

  tft.fillScreen(TFT_WHITE);

  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);

  tft.drawString("POST PROCESSING", 240, 30, 4);

  digitalWrite(CYLINDER_GATE, HIGH);
  delay(2000);  //calibrate

  // ================= BẬT PTC GIA NHIỆT =================
  digitalWrite(RELAY_PTC, LOW);
  Serial.println("RELAY PTC: ON - Heating started");

  // ===== Vẽ nút DONE để quay về menu =====
  tft.fillRoundRect(180, 200, 120, 50, 8, TFT_GREEN);
  tft.drawRoundRect(180, 200, 120, 50, 8, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_GREEN);
  tft.drawString("DONE", 240, 225, 4);
}

void postProcessingLoop() {
  uint16_t px, py;

  if (tft.getTouch(&px, &py)) {
    // vùng nút DONE trùng tọa độ vẽ ở trên
    if (px >= 180 && px <= 300 && py >= 200 && py <= 250) {
      beep();
      Serial.println("POST PROCESSING DONE -> MENU");
      currentMode = MODE_MENU;
      drawMenu();
      delay(300);
    }
  }
}