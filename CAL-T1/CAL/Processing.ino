#include "HX711.h"
#define PUMPWATER_PIN 2 
#define PUMPOIL_PIN 3 
#define HX_DT 4 
#define HX_SCK 5 

void enterProcessing() { 
currentMode = MODE_PROCESSING; 
tft.fillScreen(TFT_WHITE); 
tft.setTextColor(TFT_BLACK); 
tft.setTextDatum(TL_DATUM); 
tft.drawString("PROCESSING", 20, 20, 4); 
// ================= TARGET ================= 
tft.setTextColor(TFT_BLACK); 
tft.drawString( "Target", 20, 50, 2 ); 
tft.drawString( "Waste Oil : " + String(oilValue,1) + " g", 20, 80, 2 ); 
tft.drawString( "Water : " + String(waterValue,1) + " g", 20, 120, 2 ); 
tft.drawString( "NaOH : " + String(naohValue,1) + " g", 20, 160, 2 ); 
// ================= ACTUAL MEASURE ================= 
tft.setTextColor(TFT_DARKGREEN); 
tft.drawString( "Actual Measure", 250, 50, 2 ); 
tft.drawString( "Oil : -", 250, 80, 2 ); 
tft.drawString( "Water : -", 250, 120, 2 ); 
tft.drawString( "NaOH : -", 250, 160, 2 ); 
// ================= PUMPS ================= 
waterAdded = false; 
actualWaterWeight = 0.0; 
// reset cân với cylinder đang đặt sẵn 
scale.tare(); 
// chỉ bật pump nước 
digitalWrite(PUMPWATER_PIN, HIGH); 
digitalWrite(PUMPOIL_PIN, LOW); 
// ================= STATUS ================= 
tft.setTextColor(TFT_GREEN); 
tft.drawString( "Pump Water: ON", 20, 220, 2 ); 
tft.setTextColor(TFT_RED); 
tft.drawString( "Pump Oil: OFF", 20, 245, 2 ); 
// ================= WEIGHT ================= 
tft.setTextColor(TFT_BLUE); 
tft.drawString( "Current Weight:", 250, 220, 2 ); 
tft.drawString( "0.0 g", 250, 245, 4 ); 
} 

void processingLoop() { 
  if(millis() - lastWeightUpdate >= 1000) { 
    lastWeightUpdate = millis();
    currentWeight = scale.get_units(5);

    // ================= CURRENT WEIGHT =================
    tft.fillRect(
      250,
      245,
      180,
      35,
      TFT_WHITE
    );

    tft.setTextColor(
      TFT_BLUE,
      TFT_WHITE
    );

    tft.drawString(
      String(currentWeight,1) + " g",
      250,
      245,
      4
    );


    Serial.print("Weight: ");
    Serial.println(currentWeight);

    // ================= WATER CONTROL =================
    if(!waterAdded && currentWeight >= waterValue)
    {
      // tắt bơm nước
      digitalWrite(PUMPWATER_PIN, LOW);

      waterAdded = true;

      // lưu khối lượng thực tế
      actualWaterWeight = currentWeight;

      Serial.print("Actual Water = ");
      Serial.print(actualWaterWeight,1);
      Serial.println(" g");

      Serial.println("Adding NaOH, start mixing");

      // cập nhật trạng thái pump
      tft.fillRect(20, 220, 180, 25, TFT_WHITE);

      tft.setTextColor(TFT_RED, TFT_WHITE);
      tft.drawString(
        "Pump Water: OFF",
        20,
        220,
        2
      );

      // cập nhật Actual Measure - Water
      tft.fillRect(
        250,
        120,
        220,
        20,
        TFT_WHITE
      );

      tft.setTextColor(
        TFT_DARKGREEN,
        TFT_WHITE
      );

      tft.drawString(
        "Water : " +
        String(actualWaterWeight,1) +
        " g",
        250,
        120,
        2
      );

      // bước tiếp theo
      tft.setTextColor(TFT_RED, TFT_WHITE);

      tft.drawString(
        "Adding NaOH...",
        20,
        280,
        2
      );
    }
  }  
}
