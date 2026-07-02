#include "HX711.h"
#include <Stepper.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <DHT.h>
#define PUMPWATER_PIN 40 
#define PUMPOIL_PIN 41 
#define HX_DT 4 
#define HX_SCK 5

#define MOTOR_A1 39
#define MOTOR_A2 45

#define IN1 35
#define IN2 36
#define IN3 37
#define IN4 38

#define NAOH_GATE_SW 46

#define DHT_PIN 20
#define DHT_TYPE DHT11

const int STEPS_PER_REV = 2048;
Stepper mixerMotor(
  STEPS_PER_REV,
  IN1,
  IN3,
  IN2,
  IN4
);

bool mixingStarted = false;
bool gateReturning = false;

void enterProcessing() { 
playModeMusic(4);
processStopped = false;
currentMode = MODE_PROCESSING; 
tft.fillScreen(TFT_WHITE); 
tft.setTextColor(TFT_BLACK); 
tft.setTextDatum(TL_DATUM); 
tft.drawString("PROCESSING", 20, 20, 4); 
// ===== STOP BUTTON =====
tft.fillRoundRect(
  btnStop.x,
  btnStop.y,
  btnStop.w,
  btnStop.h,
  6,
  TFT_RED
);
tft.drawRoundRect(
  btnStop.x,
  btnStop.y,
  btnStop.w,
  btnStop.h,
  6,
  TFT_BLACK
);

tft.setTextColor(
  TFT_WHITE,
  TFT_RED
);

tft.drawCentreString(
  "STOP",
  btnStop.x + btnStop.w/2,
  btnStop.y + 12,
  2
);
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
naohAdded = false; //stepper
actualNaOHWeight = 0.0; //stepper
oilAdded = false;
actualOilWeight = 0.0;
// reset cân với cylinder đang đặt sẵn 
scale.tare(); 
// chỉ bật pump nước 
digitalWrite(PUMPWATER_PIN, HIGH); 
digitalWrite(PUMPOIL_PIN, LOW); 
// motor DC quay
digitalWrite(MOTOR_A1, LOW);
digitalWrite(MOTOR_A2, LOW);
// ================= STATUS ================= 
tft.setTextColor(TFT_GREEN); 
tft.drawString( "Pump Water: ON", 20, 220, 2 ); 
tft.setTextColor(TFT_RED); 
tft.drawString( "Pump Oil: OFF", 20, 245, 2 ); 
tft.setTextColor(TFT_BLUE);
tft.drawString("Mixer Motor: ON", 250, 280, 2);
// ================= WEIGHT ================= 
tft.setTextColor(TFT_BLUE); 
tft.drawString( "Current Weight:", 250, 220, 2 ); 
tft.drawString( "0.0 g", 250, 245, 4 ); 
// ================= STEPPER MOTOR ===================
mixingStarted = false;
mixerMotor.setSpeed(12);
} 

void processingLoop() { 
  if(processStopped)
  {
    return;
  }

  // ================= CLOSE NaOH GATE =================
  if(gateReturning)
  {
    // LOW = đang nhấn (INPUT_PULLUP)
    if(digitalRead(NAOH_GATE_SW) == LOW)
    {
        gateReturning = false;

        // tắt coil
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);

        Serial.println("NaOH GATE CLOSED");

        digitalWrite(PUMPOIL_PIN, HIGH);
        Serial.println("Adding Waste Oil");
    }
    else
    {
        mixerMotor.step(50);     // quay CCW từng bước
    }
  }

  uint16_t tx, ty;

  if(tft.getTouch(&tx, &ty))
  {
    if(inButton(btnStop, tx, ty))
    {
      Serial.println("PROCESS STOPPED");
      resetProcessing();
      currentMode = MODE_MENU;
      drawMenu();
      delay(300);
      return;
    }
  }

  if(millis() - lastWeightUpdate >= 790) { 
    lastWeightUpdate = millis();
    currentWeight = scale.get_units(5);

    humid = dht.readHumidity();
    tempt = dht.readTemperature();

  if(!isnan(humid) && !isnan(tempt))
  {
    Serial.print("Temperature: ");
    Serial.print(tempt);
    Serial.print(" C   ");

    Serial.print("Humidity: ");
    Serial.print(humid);
    Serial.println(" %");
  }
  else
  {
    Serial.println("DHT11 Read Failed");
  }

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
      if(!mixingStarted)
    {
      mixingStarted = true;

      Serial.println("Opening NaOH gate...");

      unsigned long t = millis();

      while(millis() - t < 5000)
      {
        mixerMotor.step(-1);      // clockwise
      }

      Serial.println("Gate fully opened");
    }

      // cập nhật trạng thái pump
      tft.fillRect(20, 220, 180, 25, TFT_WHITE);

      tft.setTextColor(TFT_RED, TFT_WHITE);
      tft.drawString(
        "Pump Water: OFF",
        20,
        220,
        2
      );

      tft.fillRect(
        20,
        245,
        180,
        20,
        TFT_WHITE
      );

      tft.setTextColor(
        TFT_GREEN,
        TFT_WHITE
      );

      tft.drawString(
        "Pump Oil: ON",
        20,
        245,
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

    // ================= NaOH CONTROL =================
    if(waterAdded &&
      mixingStarted &&
      !naohAdded &&
      currentWeight >= actualWaterWeight + naohValue)
    {
      naohAdded = true;

      actualNaOHWeight =
        currentWeight - actualWaterWeight;

      Serial.print("Actual NaOH = ");
      Serial.print(actualNaOHWeight,1);
      Serial.println(" g");

      Serial.println("NaOH completed");

      delay(500);   // giống code mẫu


      Serial.println("Returning mixer...");
      Serial.println("Start step back");

      gateReturning = true;

      Serial.println("Closing NaOH gate...");

      // cập nhật Actual Measure
      tft.fillRect(
        250,
        160,
        220,
        20,
        TFT_WHITE
      );

      tft.setTextColor(
        TFT_DARKGREEN,
        TFT_WHITE
      );

      tft.drawString(
        "NaOH : " +
        String(actualNaOHWeight,1) +
        " g",
        250,
        160,
        2
      );

      // cập nhật trạng thái
      tft.fillRect(
        20,
        280,
        220,
        25,
        TFT_WHITE
      );

      tft.setTextColor(
        TFT_DARKGREEN,
        TFT_WHITE
      );

      tft.drawString(
        "NaOH Completed",
        20,
        280,
        2
      );
    }

    // ================= OIL CONTROL =================
    if(naohAdded &&
      !gateReturning &&
      !oilAdded &&
      currentWeight >= actualWaterWeight +
                        actualNaOHWeight +
                        oilValue)
    {
        oilAdded = true;

        digitalWrite(PUMPOIL_PIN, LOW);

        actualOilWeight =
          currentWeight
          - actualWaterWeight
          - actualNaOHWeight;

        Serial.print("Actual Oil = ");
        Serial.print(actualOilWeight,1);
        Serial.println(" g");

        Serial.println("Oil completed");

        // cập nhật Actual Measure
        tft.fillRect(
          250,
          80,
          220,
          20,
          TFT_WHITE
        );

        tft.setTextColor(
          TFT_DARKGREEN,
          TFT_WHITE
        );

        tft.drawString(
          "Oil : " +
          String(actualOilWeight,1) +
          " g",
          250,
          80,
          2
        );

        // cập nhật trạng thái Pump Oil
        tft.fillRect(
          20,
          245,
          180,
          20,
          TFT_WHITE
        );

        tft.setTextColor(
          TFT_RED,
          TFT_WHITE
        );

        tft.drawString(
          "Pump Oil: OFF",
          20,
          245,
          2
        );

        // thông báo hoàn thành
        tft.fillRect(
          20,
          280,
          220,
          25,
          TFT_WHITE
        );

        tft.setTextColor(
          TFT_DARKGREEN,
          TFT_WHITE
        );

        tft.drawString(
          "Oil Completed",
          20,
          280,
          2
        );
      delay(5000);
      // update motor stat
      tft.fillRect(
        20,
        270,
        220,
        25,
        TFT_WHITE
      );

      tft.setTextColor(
        TFT_RED,
        TFT_WHITE
      );

      // tắt motor DC
      digitalWrite(MOTOR_A1, HIGH);
      digitalWrite(MOTOR_A2, HIGH);

      tft.drawString(
        "Mixer Motor: OFF",
        250,
        280,
        2
      );

      // =============POST PROCESSING================
      gateServo.write(90);
      delay(5000); //change when make prototype (15000)
      liftServo.write(90);

    }
  }  
}

void resetProcessing()
{
    // tắt pump
    digitalWrite(PUMPWATER_PIN, LOW);
    digitalWrite(PUMPOIL_PIN, LOW);

    // tắt motor DC
    digitalWrite(MOTOR_A1, HIGH);
    digitalWrite(MOTOR_A2, HIGH);

  // ================= HOME NaOH GATE =================
  Serial.println("Returning NaOH Gate...");

  unsigned long t0 = millis();

  while (digitalRead(NAOH_GATE_SW) != LOW)
  {
    mixerMotor.step(1);

    if (millis() - t0 > 10000)
    {
        Serial.println("ERROR: NaOH Gate timeout");
        break;
    }
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  Serial.println("NaOH GATE CLOSED");

    // reset biến
    waterAdded = false;
    naohAdded = false;
    oilAdded = false;

    mixingStarted = false;

    actualWaterWeight = 0.0;
    actualNaOHWeight  = 0.0;
    actualOilWeight   = 0.0;

    currentWeight = 0.0;
    lastWeightUpdate = 0;

    // reset màn CAL
    inputText = "";

    soapValue  = 0.0;
    oilValue   = 0.0;
    waterValue = 0.0;
    naohValue  = 0.0;

    calculated = false;
    inputError = false;

    liftServo.write(0);
    gateServo.write(0);

    tempt = 0.0;
    humid = 0.0;
}