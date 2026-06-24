#include <TFT_eSPI.h>
#include <HX711.h>
#define BG_COLOR      0xEF7D   // light gray
#define CARD_COLOR    TFT_WHITE
#define INPUT_COLOR   0xD6FF   // light blue
#define ENTER_COLOR   0xA7F3   // light green
#define BACK_COLOR    0xFBAE   // light red
#define BTN_COLOR     TFT_WHITE
#define BORDER_COLOR  0x8410   // gray
#define TEXT_COLOR    TFT_BLACK

#define MOTOR_A1 39
#define MOTOR_A2 45

#define PUMPWATER_PIN   40
#define PUMPOIL_PIN 41

#define HX_DT   4
#define HX_SCK  5

HX711 scale;
float currentWeight = 0.0;
bool waterAdded = false;
unsigned long lastWeightUpdate = 0;

TFT_eSPI tft = TFT_eSPI();

// ================= TOUCH =================
uint16_t tx, ty;

// ================= INPUT =================
String inputText = "";

float soapValue  = 0.0;
float waterValue = 0.0;
float naohValue  = 0.0;
float oilValue   = 0.0;
float actualWaterWeight = 0.0;
float actualNaOHWeight = 0.0;
float actualOilWeight = 0.0;

bool oilAdded = false;
bool naohAdded = false;
bool inputError = false;
bool calculated = false;

enum ScreenMode {
  MODE_CAL,
  MODE_PROCESSING
};

ScreenMode currentMode = MODE_CAL;

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

// ==========================================
bool inButton(Button &b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w &&
          y >= b.y && y <= b.y + b.h);
}

// ==========================================
void calculateSoap() {

  soapValue = inputText.toFloat();

  oilValue   = soapValue * 700.0 / 792.0;
  waterValue = soapValue * 260.0 / 792.0;
  naohValue  = soapValue * 92.0  / 792.0;
}

// ==========================================
void drawInfo() {

  tft.fillRoundRect(10, 10, 160, 305, 8, CARD_COLOR);
  tft.drawRoundRect(10, 10, 160, 305, 8, BORDER_COLOR);

  tft.setTextColor(TEXT_COLOR, CARD_COLOR);

  tft.setTextDatum(TC_DATUM);
  tft.drawString("INFO", 90, 20, 4);

  tft.setTextDatum(TL_DATUM);

  if(inputError)
  {
    tft.setTextColor(TFT_RED, CARD_COLOR);

    tft.drawString("Soap", 20, 50, 2);
    tft.drawString("90-1000 g", 20, 70, 2);
  }
  else
  {
    tft.setTextColor(TFT_BLUE, CARD_COLOR);

    tft.drawString("Soap", 20, 50, 2);
    tft.drawString(String(soapValue,1)+" g", 20, 70, 4);
  }

  tft.setTextColor(TFT_DARKGREEN, CARD_COLOR);
  tft.drawString("Waste Oil", 20, 110, 2);
  tft.drawString(String(oilValue,1)+" g", 20, 130, 4);

  tft.setTextColor(TFT_NAVY, CARD_COLOR);
  tft.drawString("Water", 20, 170, 2);
  tft.drawString(String(waterValue,1)+" g", 20, 190, 4);

  tft.setTextColor(TFT_RED, CARD_COLOR);
  tft.drawString("NaOH", 20, 230, 2);
  tft.drawString(String(naohValue,1)+" g", 20, 250, 4);
}
// ==========================================
void drawInputBox() {

  tft.fillRoundRect(175, 10, 220, 55, 8, INPUT_COLOR);
  tft.drawRoundRect(175, 10, 220, 55, 8, BORDER_COLOR);

  tft.setTextDatum(MC_DATUM);

  String showText = inputText;

  if(showText=="")
    showText="0.0";

  tft.setTextColor(TEXT_COLOR, INPUT_COLOR);

  tft.drawString(showText, 285, 40, 6);
}

// ==========================================
void drawButton(Button &b) {

  uint16_t fill = BTN_COLOR;

  if(b.label=="Calculate")
    fill = ENTER_COLOR;

  if(b.label=="Delete")
    fill = BACK_COLOR;

  if(b.label=="Home")
    fill = TFT_YELLOW;

  if(b.label=="OK")
  {
    if(calculated)
      fill = TFT_CYAN;
    else
      fill = TFT_LIGHTGREY;
  }

  tft.fillRoundRect(
      b.x,b.y,b.w,b.h,
      6,
      fill
  );

  tft.drawRoundRect(
      b.x,b.y,b.w,b.h,
      6,
      BORDER_COLOR
  );

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TEXT_COLOR, fill);

  int font = 4;

  if(b.label=="Calculate")
    font = 2;
  if(b.label=="Home")
    font = 2;
  if(b.label=="OK")
    font = 2;
  if(b.label=="Delete")
    font = 2;
  tft.drawString(
      b.label,
      b.x + b.w/2,
      b.y + b.h/2,
      font
  );
}

// ==========================================
void drawUI() {

  tft.fillScreen(BG_COLOR);

  drawInfo();
  drawInputBox();

  for(int i=0;i<11;i++)
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

  numBtns[0] = {175, 75,  bw, bh, "7"};
  numBtns[1] = {255, 75,  bw, bh, "8"};
  numBtns[2] = {335, 75,  bw, bh, "9"};

  numBtns[3] = {175, 140, bw, bh, "4"};
  numBtns[4] = {255, 140, bw, bh, "5"};
  numBtns[5] = {335, 140, bw, bh, "6"};

  numBtns[6] = {175, 205, bw, bh, "1"};
  numBtns[7] = {255, 205, bw, bh, "2"};
  numBtns[8] = {335, 205, bw, bh, "3"};

  numBtns[9]  = {175, 270, 150, 45, "0"};
  numBtns[10] = {335, 270, 70,  45, "."};

  btnBack  = {410, 75, 65, 115, "Delete"};
  btnEnter = {410, 200, 65, 115, "Calculate"};
  btnHome    = {410, 10, 65, 55, "Home"};
  btnConfirm = {95, 275, 70, 35, "OK"};
}

// ==========================================
void refreshScreen() {

  drawInfo();
  drawInputBox();
  drawButton(btnConfirm);
}

// ==========================================
void setup() {

  Serial.begin(115200);

  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.setTextFont(2);
  tft.setTextPadding(0);

  pinMode(PUMPWATER_PIN, OUTPUT);
  pinMode(PUMPOIL_PIN, OUTPUT);

  pinMode(MOTOR_A1, OUTPUT);
  pinMode(MOTOR_A2, OUTPUT);

  digitalWrite(MOTOR_A1, HIGH);
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

  drawUI();

  scale.begin(HX_DT, HX_SCK);
  scale.set_scale(-402.9);  //CALIBRATION
  scale.tare();

  Serial.println("HX711 READY");
  Serial.println("SOAP CAL READY");

  Serial.println("SOAP CAL READY");
}

// ==========================================
void loop() {
  if(currentMode == MODE_PROCESSING)
  {
    processingLoop();
    return;
  }

  bool pressed = tft.getTouch(&tx, &ty);

  if(!pressed)
    return;

  // ---------- NUMPAD ----------
  for(int i = 0; i < 11; i++) {

    if(inButton(numBtns[i], tx, ty)) {

      String v = numBtns[i].label;

      // ===== DOT =====
      if(v == ".") {

        if(inputText.length() == 0)
          inputText = "0.";

        else if(inputText.indexOf('.') == -1)
          inputText += ".";
      }
      else {

        String test = inputText + v;

        // max 1000.0
        if(test.length() <= 6 &&
           test.toFloat() <= 1000.0) {

          int dotPos = test.indexOf('.');

          if(dotPos >= 0) {

            int decimals =
              test.length() - dotPos - 1;

            if(decimals <= 1)
              inputText = test;
          }
          else {
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
  if(inButton(btnHome, tx, ty)) {

    Serial.println("HOME");

    delay(300);
    return;
  }

  // ---------- CONFIRM ----------
  if(inButton(btnConfirm, tx, ty))
  {
    if(!calculated)
    {
      Serial.println("PLEASE CALCULATE FIRST");

      delay(300);
      return;
    }

    Serial.println("CONFIRM");
    enterProcessing();
    // code chuyển màn hình ở đây

    delay(300);
    return;
  }

  // ---------- BACKSPACE ----------
  if(inButton(btnBack, tx, ty)) {

    if(inputText.length() > 0)
      inputText.remove(inputText.length() - 1);

    calculated = false;    
    drawInputBox();    
    drawButton(btnConfirm);

    delay(200);
    return;
  }

  // ---------- ENTER ----------
  if(inButton(btnEnter, tx, ty)) {

    if(inputText.length() > 0) {

    float value = inputText.toFloat();

    if(value < 90.0 || value > 1000.0)
    {
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