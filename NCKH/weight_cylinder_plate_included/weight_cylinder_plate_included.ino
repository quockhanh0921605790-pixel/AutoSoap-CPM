#include "HX711.h"

#define HX711_DOUT 4
#define HX711_SCK  5

HX711 scale;

// Calibration factor của m
float calibration_factor = -404.0;

void setup() {
  Serial.begin(115200);

  scale.begin(HX711_DOUT, HX711_SCK);
  scale.set_scale(calibration_factor);

  Serial.println("HX711 GRAM TEST");
  Serial.println("Keep mica plate + cylinder on scale...");

  delay(3000);

  // Mica + bình trụ = 0g
  scale.tare();

  Serial.println("TARE COMPLETE");
}

void loop() {

  // Lấy trung bình 10 mẫu
  float weight = scale.get_units(10);

  // Chống nhiễu
  if(weight < 0.5)
    weight = 0;

  Serial.print("Weight: ");
  Serial.print(weight, 2);
  Serial.println(" g");

  delay(500);
}