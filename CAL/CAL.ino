//=====CAL.ino file=====

#include <TFT_eSPI.h>
#include <HX711.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Stepper.h>
#define BG_COLOR 0xEF7D  // light gray
#define CARD_COLOR TFT_WHITE
#define INPUT_COLOR 0xD6FF  // light blue
#define ENTER_COLOR 0xA7F3  // light green
#define BACK_COLOR 0xFBAE   // light red
#define BTN_COLOR TFT_WHITE
#define BORDER_COLOR 0x8410  // gray
#define TEXT_COLOR TFT_BLACK

#define MOTOR_A2 21

#define CYLINDER_GATE 16

#define PUMPWATER_PIN 42
#define PUMPOIL_PIN 1

#define HX_DT 4
#define HX_SCK 5

#define NAOH_GATE_SW 46

#define DS18B20_PIN 14

#define BUZZER_PIN 2

#define RELAY_PTC 15

#define IN1 38
#define IN2 39
#define IN3 40
#define IN4 41
extern Stepper mixerMotor;
// ==========================================
void beep(int duration = 50);   // <-- prototype, calibrate

OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

HX711 scale;
HardwareSerial mp3Serial(1);
DFRobotDFPlayerMini mp3;

float tempt = 0.0;
float currentWeight = 0.0;
bool waterAdded = false;
unsigned long lastWeightUpdate = 0;

TFT_eSPI tft = TFT_eSPI();

// ================= TOUCH =================
uint16_t tx, ty;

// ================= INPUT =================
String inputText = "";

float soapValue = 0.0;
float waterValue = 0.0;
float naohValue = 0.0;
float oilValue = 0.0;
float actualWaterWeight = 0.0;
float actualNaOHWeight = 0.0;
float actualOilWeight = 0.0;

bool oilAdded = false;
bool naohAdded = false;
bool inputError = false;
bool calculated = false;
bool lastGateState = false;

int currentTrack = 0;

enum ScreenMode {
  MODE_INTRO,
  MODE_MENU,
  MODE_CAL,
  MODE_PROCESSING,
  MODE_POST_PROCESSING
};

ScreenMode currentMode = MODE_INTRO;

// ================= BUTTON =================
struct Button {
  int x;
  int y;
  int w;
  int h;
  String label;
};

Button numBtns[11];
Button btnBack;
Button btnEnter;
Button btnConfirm;
Button btnHome;
Button btnStop = {
  380,  // x
  10,   // y
  90,   // w
  40,   // h
  "STOP"
};

bool processStopped = false;

// ==========================================
bool inButton(Button &b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h);
}

// ==========================================
void calculateSoap() {
  soapValue = inputText.toFloat();

  oilValue   = soapValue * 700.0 / 999.78;
  waterValue = soapValue * 210.0 / 999.78;
  naohValue  = soapValue * 89.78 / 999.78;
}

void playModeMusic(int track) {
  if (currentTrack == track)
    return;  // đang phát bài này rồi

  currentTrack = track;

  mp3.stop();
  delay(100);

  mp3.playFolder(1, track);

  Serial.print("Playing track: ");
  Serial.println(track);
}

// ==========================================
void drawInfo() {

  tft.fillRoundRect(10, 10, 160, 305, 8, CARD_COLOR);
  tft.drawRoundRect(10, 10, 160, 305, 8, BORDER_COLOR);

  tft.setTextColor(TEXT_COLOR, CARD_COLOR);

  tft.setTextDatum(TC_DATUM);
  tft.drawString("INFO", 90, 20, 4);

  tft.setTextDatum(TL_DATUM);

  if (inputError) {
    tft.setTextColor(TFT_RED, CARD_COLOR);

    tft.drawString("Soap", 20, 50, 2);
    tft.drawString("300-500 g", 20, 70, 2);
  } else {
    tft.setTextColor(TFT_BLUE, CARD_COLOR);

    tft.drawString("Soap", 20, 50, 2);
    tft.drawString(String(soapValue, 1) + " g", 20, 70, 4);
  }

  tft.setTextColor(TFT_DARKGREEN, CARD_COLOR);
  tft.drawString("Waste Oil", 20, 110, 2);
  tft.drawString(String(oilValue, 1) + " g", 20, 130, 4);

  tft.setTextColor(TFT_NAVY, CARD_COLOR);
  tft.drawString("Water", 20, 170, 2);
  tft.drawString(String(waterValue, 1) + " g", 20, 190, 4);

  tft.setTextColor(TFT_RED, CARD_COLOR);
  tft.drawString("NaOH", 20, 230, 2);
  tft.drawString(String(naohValue, 1) + " g", 20, 250, 4);
}
// ==========================================
void drawInputBox() {

  tft.fillRoundRect(175, 10, 220, 55, 8, INPUT_COLOR);
  tft.drawRoundRect(175, 10, 220, 55, 8, BORDER_COLOR);

  tft.setTextDatum(MC_DATUM);

  String showText = inputText;

  if (showText == "")
    showText = "0.0";

  tft.setTextColor(TEXT_COLOR, INPUT_COLOR);

  tft.drawString(showText, 285, 40, 6);
}

// ==========================================
void drawButton(Button &b) {

  uint16_t fill = BTN_COLOR;

  if (b.label == "Calculate")
    fill = ENTER_COLOR;

  if (b.label == "Delete")
    fill = BACK_COLOR;

  if (b.label == "Home")
    fill = TFT_YELLOW;

  if (b.label == "OK") {
    if (calculated)
      fill = TFT_CYAN;
    else
      fill = TFT_LIGHTGREY;
  }

  tft.fillRoundRect(
    b.x, b.y, b.w, b.h,
    6,
    fill);

  tft.drawRoundRect(
    b.x, b.y, b.w, b.h,
    6,
    BORDER_COLOR);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TEXT_COLOR, fill);

  int font = 4;

  if (b.label == "Calculate")
    font = 2;
  if (b.label == "Home")
    font = 2;
  if (b.label == "OK")
    font = 2;
  if (b.label == "Delete")
    font = 2;
  tft.drawString(
    b.label,
    b.x + b.w / 2,
    b.y + b.h / 2,
    font);
}

// ==========================================
void drawUI() {
  playModeMusic(3);

  tft.fillScreen(BG_COLOR);

  drawInfo();
  drawInputBox();

  for (int i = 0; i < 11; i++)
    drawButton(numBtns[i]);

  drawButton(btnBack);
  drawButton(btnEnter);
  drawButton(btnConfirm);
  drawButton(btnHome);
}

// ==========================================
void setupButtons() {

  int bw = 70;
  int bh = 55;

  numBtns[0] = { 175, 75, bw, bh, "7" };
  numBtns[1] = { 255, 75, bw, bh, "8" };
  numBtns[2] = { 335, 75, bw, bh, "9" };

  numBtns[3] = { 175, 140, bw, bh, "4" };
  numBtns[4] = { 255, 140, bw, bh, "5" };
  numBtns[5] = { 335, 140, bw, bh, "6" };

  numBtns[6] = { 175, 205, bw, bh, "1" };
  numBtns[7] = { 255, 205, bw, bh, "2" };
  numBtns[8] = { 335, 205, bw, bh, "3" };

  numBtns[9] = { 175, 270, 150, 45, "0" };
  numBtns[10] = { 335, 270, 70, 45, "." };

  btnBack = { 410, 75, 65, 115, "Delete" };
  btnEnter = { 410, 200, 65, 115, "Calculate" };
  btnHome = { 410, 10, 65, 55, "Home" };
  btnConfirm = { 95, 275, 70, 35, "OK" };
}

// ==========================================
void refreshScreen() {

  drawInfo();
  drawInputBox();
  drawButton(btnConfirm);
}

// ==========================================
void showIntro() {
  playModeMusic(1);  // 01/001.mp3

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
  tft.drawString("AutoSoap CPM", 240, 140, 4);

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
    TFT_DARKGREY);

  unsigned long startTime = millis();
  int oldProgress = -1;

  while (millis() - startTime < 5000) {
    int progress =
      map(
        millis() - startTime,
        0,
        5000,
        0,
        w - 4);

    if (progress != oldProgress) {
      tft.fillRoundRect(
        x + 2,
        y + 2,
        progress,
        h - 4,
        6,
        TFT_CYAN);

      oldProgress = progress;
    }

    tft.setTextColor(
      TFT_NAVY,
      0xEFFF);

    tft.drawString(
      "Loading...",
      240,
      260,
      2);
  }

  // ===== Transition =====
  tft.fillScreen(TFT_WHITE);
  delay(100);

  tft.fillScreen(0xCFFF);
  delay(100);

  // Chuyển sang MENU
  currentMode = MODE_MENU;
}

void drawMenu() {
  playModeMusic(2);
  tft.fillScreen(TFT_WHITE);

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(TFT_BLACK);
  tft.drawString("MENU", 240, 40, 4);

  // ONE TIME
  tft.fillRoundRect(
    140,
    200,
    200,
    70,
    10,
    TFT_GREEN);

  tft.drawString(
    "BAT DAU",
    240,
    235,
    4);
}

// ==========================================
void setup() {

  Serial.begin(115200);

  ds18b20.begin();

  mp3Serial.begin(
    9600,
    SERIAL_8N1,
    18,  // RX ESP32 <- TX DFPlayer
    17   // TX ESP32 -> RX DFPlayer
  );

  if (mp3.begin(mp3Serial)) {
    Serial.println("DFPlayer OK");

    mp3.volume(25);  // 0~30
  } else {
    Serial.println("DFPlayer FAIL");
  }

  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.setTextFont(2);
  tft.setTextPadding(0);

  pinMode(CYLINDER_GATE, OUTPUT);

  pinMode(PUMPWATER_PIN, OUTPUT);
  pinMode(PUMPOIL_PIN, OUTPUT);

  pinMode(MOTOR_A2, OUTPUT);
  pinMode(NAOH_GATE_SW, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(RELAY_PTC, OUTPUT_OPEN_DRAIN);
  digitalWrite(RELAY_PTC, HIGH);  
  // ================= HOME NaOH GATE ON BOOT =================
  mixerMotor.setSpeed(15);
  Serial.println("Homing NaOH Gate...");

  unsigned long homingStart = millis();

  while (digitalRead(NAOH_GATE_SW) != LOW) {
    mixerMotor.step(-1);

    if (millis() - homingStart > 10000) {
      Serial.println("ERROR: NaOH Gate homing timeout at boot");
      break;
    }
  }

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  Serial.println("NaOH GATE CLOSED (homed)");

  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(CYLINDER_GATE, LOW); 
  delay(800);  // calibrate

  digitalWrite(MOTOR_A2, HIGH);

  digitalWrite(PUMPWATER_PIN, LOW);
  digitalWrite(PUMPOIL_PIN, LOW);

  uint16_t calData[5] = {
    263, 3638,
    227, 3620,
    7
  };

  tft.setTouch(calData);

  setupButtons();

  showIntro();

  if (currentMode == MODE_MENU) {
    drawMenu();
  }

  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(-450);  //CALIBRATION -402.9
  scale.tare();

  Serial.println("HX711 READY");
  Serial.println("SOAP CAL READY");

  Serial.println("SOAP CAL READY");
}

// ==========================================
void loop() {

  bool gateClosed = (digitalRead(NAOH_GATE_SW) == LOW);

  if (gateClosed != lastGateState) {
    lastGateState = gateClosed;

    if (gateClosed)
      Serial.println("NaOH GATE CLOSED");
    else
      Serial.println("NaOH GATE OPEN");
  }

  if (currentMode == MODE_PROCESSING) {
    processingLoop();
    return;
  }

  if (currentMode == MODE_POST_PROCESSING) {
    postProcessingLoop();
    return;
  }

  bool pressed = tft.getTouch(&tx, &ty);

  if (currentMode == MODE_MENU) {
    if (!pressed)
      return;

    // ONE TIME
    if (tx >= 140 && tx <= 340 && ty >= 200 && ty <= 270) {
      beep();
      Serial.println("ONE TIME");

      currentMode = MODE_CAL;

      drawUI();

      delay(300);
    }

    return;
  }

  if (currentMode != MODE_CAL)
    return;

  if (!pressed)
    return;

  // ---------- NUMPAD ----------
  for (int i = 0; i < 11; i++) {

    if (inButton(numBtns[i], tx, ty)) {
      beep(); 
      String v = numBtns[i].label;

      // ===== DOT =====
      if (v == ".") {

        if (inputText.length() == 0)
          inputText = "0.";

        else if (inputText.indexOf('.') == -1)
          inputText += ".";
      } else {

        String test = inputText + v;

        // max 1000.0
        if (test.length() <= 6 && test.toFloat() <= 500.0) {

          int dotPos = test.indexOf('.');

          if (dotPos >= 0) {

            int decimals =
              test.length() - dotPos - 1;

            if (decimals <= 1)
              inputText = test;
          } else {
            inputText = test;
          }
        }
      }
      calculated = false;
      drawInputBox();
      drawButton(btnConfirm);

      delay(200);
      return;
    }
  }
  // ---------- HOME ----------
  if (inButton(btnHome, tx, ty)) {
    beep(); 
    Serial.println("HOME");
    currentMode = MODE_MENU;
    drawMenu();
    delay(300);
    return;
  }

  // ---------- CONFIRM ----------
  if (inButton(btnConfirm, tx, ty)) {
    if (!calculated) {
      Serial.println("PLEASE CALCULATE FIRST");
      beep();
      delay(100);
      beep();
      delay(100);
      beep();
      delay(100);
      return;
    }

    Serial.println("CONFIRM");
    beep(); 
    enterProcessing();
    // code chuyển màn hình ở đây

    delay(300);
    return;
  }

  // ---------- BACKSPACE ----------
  if (inButton(btnBack, tx, ty)) {
    beep(); 
    if (inputText.length() > 0)
      inputText.remove(inputText.length() - 1);

    calculated = false;
    drawInputBox();
    drawButton(btnConfirm);

    delay(200);
    return;
  }

  // ---------- ENTER ----------
  if (inButton(btnEnter, tx, ty)) {
    beep(); 
    if (inputText.length() > 0) {

      float value = inputText.toFloat();

      if (value < 300.0 || value > 500.0) {
        inputError = true;
        calculated = false;
        refreshScreen();

        delay(300);
        return;
      }

      inputError = false;

      calculateSoap();
      calculated = true;
      refreshScreen();

      Serial.print("SOAP = ");
      Serial.println(soapValue);

      Serial.print("WASTE OIL = ");
      Serial.println(oilValue);

      Serial.print("WATER = ");
      Serial.println(waterValue);

      Serial.print("NAOH = ");
      Serial.println(naohValue);
    }

    delay(300);
    return;
  }
}

// ==========================================
void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}