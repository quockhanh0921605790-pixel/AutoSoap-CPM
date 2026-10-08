#include <OneWire.h>
#include <DallasTemperature.h>
#include <TFT_eSPI.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>

TFT_eSPI tft = TFT_eSPI();
HardwareSerial mp3Serial(1);
DFRobotDFPlayerMini mp3;

int currentTrack = 0;
bool mp3Ready = false;

uint16_t tx, ty;
bool lastPressed = false;


// =====================================================
// COLORS
// =====================================================

#define BG_COLOR      0xEF7D
#define CARD_COLOR    TFT_WHITE
#define INPUT_COLOR   0xD6FF
#define ENTER_COLOR   0xA7F3
#define BACK_COLOR    0xFBAE
#define BTN_COLOR     TFT_WHITE
#define BORDER_COLOR  0x8410
#define TEXT_COLOR    TFT_BLACK

//=======MODULES=======
#define PUMPOIL_PIN   1
#define PUMPNAOH_PIN  42
#define MIXER_PIN 21
#define DS18B20_PIN 14

OneWire oneWire(DS18B20_PIN);
DallasTemperature temperatureSensor(&oneWire);

// =====================================================
// MODES
// =====================================================

enum ScreenMode {
  MODE_INTRO,
  MODE_MENU,
  MODE_CAL,
  MODE_PROCESSING,
  MODE_SETTINGS,
  MODE_TIMER_CALIBRATION
};

ScreenMode currentMode = MODE_INTRO;


// =====================================================
// CAL DATA
// =====================================================

String inputText = "";

float soapValue  = 0.0;
float oilValue   = 0.0;
float naohSolutionValue = 0.0;

bool inputError = false;
bool calculated = false;


// =====================================================
// BUTTON
// =====================================================

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
Button btnSettings;
Button btnTimerCalibration;
Button btnSettingsBack;

// =====================================================
// TIMER CALIBRATION
// =====================================================

enum TimerCalibrationState {
  TC_WAIT_START,
  TC_OIL_RUNNING,
  TC_WAIT_NAOH,
  TC_NAOH_RUNNING
};

TimerCalibrationState timerCalibrationState =
  TC_WAIT_START;

unsigned long timerCalibrationTimer = 0;


// =====================================================
// PROTOTYPES
// =====================================================

void playModeMusic(int track) {

  if (!mp3Ready) return;

  // Không phát lại nếu đang đúng track
  if (currentTrack == track) return;

  currentTrack = track;

  mp3.stop();
  delay(100);

  // Giữ nguyên cấu trúc file cũ:
  // /01/001.mp3
  // /01/002.mp3
  // /01/003.mp3
  mp3.playFolder(1, track);

  Serial.print("Playing track: ");
  Serial.println(track);
}


void enterIntro();
void enterMenu();
void enterCal();
void enterSettings();
void enterTimerCalibration();

void introLoop();
void menuLoop(bool justPressed);
void calLoop(bool justPressed);
void settingsLoop(bool justPressed);
void timerCalibrationLoop();

void setPumpOil(bool state);
void setPumpNaOH(bool state);
void setMixer(bool state);

// =====================================================
// BUTTON HIT TEST
// =====================================================

bool inButton(Button &b, int x, int y) {

  return (
    x >= b.x &&
    x <= b.x + b.w &&
    y >= b.y &&
    y <= b.y + b.h
  );
}


// =====================================================
// CALCULATION
// =====================================================

void calculateSoap() {

  soapValue = inputText.toFloat();

  oilValue =
    soapValue * 700.0 / 999.78;

  // Dung dịch NaOH 30%
  // Gộp phần nước 210 + NaOH 89.78
  naohSolutionValue =
    soapValue * 299.78 / 999.78;
}

// =====================================================
// SETUP BUTTONS
// =====================================================

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

  btnBack = {
    410, 75,
    65, 115,
    "Delete"
  };

  btnEnter = {
    410, 200,
    65, 115,
    "Calculate"
  };

  btnHome = {
    410, 10,
    65, 55,
    "Home"
  };

  btnConfirm = {
    95, 275,
    70, 35,
    "OK"
  };

  // SETTINGS BUTTON
  btnSettings = {
  350, 275,
  120, 35,
  "Settings"
  };

// TIMER CALIBRATION
  btnTimerCalibration = {
  120, 100,
  240, 70,
  "Timer Calibration"
  };

// BACK SETTINGS
  btnSettingsBack = {
  20, 260,
  100, 45,
  "Back"
  };
}


// =====================================================
// DRAW BUTTON
// =====================================================

void drawButton(Button &b) {

  uint16_t fill = BTN_COLOR;

  if (b.label == "Calculate")
    fill = ENTER_COLOR;

  if (b.label == "Delete")
    fill = BACK_COLOR;

  if (b.label == "Home")
    fill = TFT_YELLOW;

  if (b.label == "Settings")
  fill = TFT_LIGHTGREY;

  if (b.label == "Timer Calibration")
  fill = TFT_CYAN;

  if (b.label == "Back")
  fill = BACK_COLOR;

  if (b.label == "OK") {

    if (calculated)
      fill = TFT_CYAN;
    else
      fill = TFT_LIGHTGREY;
  }


  tft.fillRoundRect(
    b.x,
    b.y,
    b.w,
    b.h,
    6,
    fill
  );

  tft.drawRoundRect(
    b.x,
    b.y,
    b.w,
    b.h,
    6,
    BORDER_COLOR
  );


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TEXT_COLOR,
    fill
  );


  int font = 4;

  if (
    b.label == "Calculate" ||
    b.label == "Delete" ||
    b.label == "Home" ||
    b.label == "OK" ||
    b.label == "Settings" ||
    b.label == "Timer Calibration" ||
    b.label == "Back"
  ) {
    font = 2;
  }


  tft.drawString(
    b.label,
    b.x + b.w / 2,
    b.y + b.h / 2,
    font
  );
}


// =====================================================
// INFO PANEL
// =====================================================

void drawInfo() {

  tft.fillRoundRect(
    10, 10,
    160, 305,
    8,
    CARD_COLOR
  );

  tft.drawRoundRect(
    10, 10,
    160, 305,
    8,
    BORDER_COLOR
  );


  tft.setTextDatum(TC_DATUM);

  tft.setTextColor(
    TEXT_COLOR,
    CARD_COLOR
  );

  tft.drawString(
    "INFO",
    90,
    20,
    4
  );


  tft.setTextDatum(TL_DATUM);


  // SOAP
  if (inputError) {

    tft.setTextColor(
      TFT_RED,
      CARD_COLOR
    );

    tft.drawString(
      "Soap",
      20, 50,
      2
    );

    tft.drawString(
      "300-500 g",
      20, 70,
      2
    );

  }

  else {

    tft.setTextColor(
      TFT_BLUE,
      CARD_COLOR
    );

    tft.drawString(
      "Soap",
      20, 50,
      2
    );

    tft.drawString(
      String(soapValue, 1) + " g",
      20, 70,
      4
    );
  }


  // OIL
  tft.setTextColor(
    TFT_DARKGREEN,
    CARD_COLOR
  );

  tft.drawString(
    "Waste Oil",
    20, 110,
    2
  );

  tft.drawString(
    String(oilValue, 1) + " g",
    20, 130,
    4
  );

  // NAOH SOLUTION 30%
  tft.setTextColor(
  TFT_RED,
  CARD_COLOR
  );

  tft.drawString(
  "NaOH Solution",
  20, 170,
  2
  );

  tft.drawString(
  "30%",
  20, 190,
  2
  );

  tft.drawString(
  String(naohSolutionValue, 1) + " g",
  20, 215,
  4
  );

}


// =====================================================
// INPUT BOX
// =====================================================

void drawInputBox() {

  tft.fillRoundRect(
    175, 10,
    220, 55,
    8,
    INPUT_COLOR
  );

  tft.drawRoundRect(
    175, 10,
    220, 55,
    8,
    BORDER_COLOR
  );


  String showText = inputText;

  if (showText == "")
    showText = "0.0";


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TEXT_COLOR,
    INPUT_COLOR
  );

  tft.drawString(
    showText,
    285,
    40,
    6
  );
}


// =====================================================
// REFRESH CAL
// =====================================================

void refreshCal() {

  drawInfo();
  drawInputBox();
  drawButton(btnConfirm);
}


// =====================================================
// MODE 1 — INTRO
// =====================================================

void enterIntro() {

  currentMode = MODE_INTRO;

  Serial.println("ENTERING MODE INTRO");
  playModeMusic(1);

  tft.fillScreen(0xEFFF);


  // LOGO
  tft.fillCircle(
    240,
    80,
    35,
    TFT_CYAN
  );

  tft.drawCircle(
    240,
    80,
    35,
    TFT_BLUE
  );


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_WHITE,
    TFT_CYAN
  );

  tft.drawString(
    "R",
    240,
    80,
    6
  );


  // TITLE
  tft.setTextColor(
    TFT_BLACK,
    0xEFFF
  );

  tft.drawString(
    "AutoSoap CPM V2",
    240,
    140,
    4
  );


  tft.setTextColor(
    TFT_DARKGREY,
    0xEFFF
  );

  tft.drawString(
    "By Khanh & Nguyen",
    240,
    170,
    2
  );


  // PROGRESS BAR
  int x = 70;
  int y = 220;
  int w = 340;
  int h = 22;


  tft.drawRoundRect(
    x, y,
    w, h,
    8,
    TFT_DARKGREY
  );


  unsigned long startTime = millis();

  int oldProgress = -1;


  while (
    millis() - startTime < 5000
  ) {

    int progress = map(
      millis() - startTime,
      0,
      5000,
      0,
      w - 4
    );


    if (progress != oldProgress) {

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

    delay(5);
  }


  Serial.println(
    "INTRO -> MENU"
  );


  enterMenu();
}


// =====================================================
// INTRO LOOP
// =====================================================

void introLoop() {

  // Intro hiện chạy một lần
  // trong enterIntro().
}


// =====================================================
// MODE 2 — MENU
// =====================================================

void enterMenu() {

  currentMode = MODE_MENU;

  Serial.println(
    "ENTERING MODE MENU"
  );
  playModeMusic(2);


  tft.fillScreen(
    TFT_WHITE
  );


  tft.setTextDatum(
    MC_DATUM
  );


  tft.setTextColor(
    TFT_BLACK,
    TFT_WHITE
  );


  tft.drawString(
    "MENU",
    240,
    60,
    4
  );


  // START BUTTON
  tft.fillRoundRect(
    140,
    200,
    200,
    70,
    10,
    TFT_GREEN
  );


  tft.setTextColor(
    TFT_BLACK,
    TFT_GREEN
  );


  tft.drawString(
    "START",
    240,
    235,
    4
  );

  // SETTINGS - góc dưới bên phải
  drawButton(btnSettings);
}


// =====================================================
// MENU LOOP
// =====================================================

void menuLoop(bool justPressed) {

  if (!justPressed)
    return;

  Serial.print("MENU TOUCH X=");
  Serial.print(tx);

  Serial.print(" Y=");
  Serial.println(ty);


  // ===================================================
  // START
  // ===================================================

  if (
    tx >= 140 &&
    tx <= 340 &&
    ty >= 200 &&
    ty <= 270
  ) {

    Serial.println("START PRESSED");

    enterCal();

    return;
  }


  // ===================================================
  // SETTINGS
  // ===================================================

  if (
    inButton(
      btnSettings,
      tx,
      ty
    )
  ) {

    Serial.println("SETTINGS PRESSED");

    enterSettings();

    return;
  }
}

// =====================================================
// MODE 3 — CAL
// =====================================================

void enterCal() {

  currentMode = MODE_CAL;

  Serial.println(
    "ENTERING MODE CAL"
  );
  playModeMusic(3);

  // Reset khi vào CAL
  inputText = "";

  soapValue = 0.0;
  oilValue = 0.0;
  naohSolutionValue = 0.0;

  inputError = false;
  calculated = false;


  // DRAW SCREEN
  tft.fillScreen(
    BG_COLOR
  );


  drawInfo();

  drawInputBox();


  for (
    int i = 0;
    i < 11;
    i++
  ) {

    drawButton(
      numBtns[i]
    );
  }


  drawButton(btnBack);
  drawButton(btnEnter);
  drawButton(btnConfirm);
  drawButton(btnHome);
}


// =====================================================
// CAL LOOP
// =====================================================

void calLoop(bool justPressed) {

  if (!justPressed)
    return;


  Serial.print(
    "CAL TOUCH X="
  );

  Serial.print(tx);

  Serial.print(" Y=");

  Serial.println(ty);


  // ===================================================
  // NUMPAD
  // ===================================================

  for (
    int i = 0;
    i < 11;
    i++
  ) {

    if (
      inButton(
        numBtns[i],
        tx,
        ty
      )
    ) {

      String v =
        numBtns[i].label;


      Serial.print(
        "KEY: "
      );

      Serial.println(v);


      // DOT
      if (v == ".") {

        if (
          inputText.length() == 0
        ) {

          inputText = "0.";
        }

        else if (
          inputText.indexOf('.') == -1
        ) {

          inputText += ".";
        }
      }


      // NUMBER
      else {

        String test =
          inputText + v;


        if (
          test.length() <= 6 &&
          test.toFloat() <= 500.0
        ) {

          int dotPos =
            test.indexOf('.');


          if (dotPos >= 0) {

            int decimals =
              test.length()
              - dotPos
              - 1;


            if (decimals <= 1) {

              inputText =
                test;
            }
          }

          else {

            inputText =
              test;
          }
        }
      }


      calculated = false;
      inputError = false;


      drawInputBox();

      drawButton(
        btnConfirm
      );


      return;
    }
  }


  // ===================================================
  // HOME
  // ===================================================

  if (
    inButton(
      btnHome,
      tx,
      ty
    )
  ) {

    Serial.println(
      "HOME PRESSED"
    );

    enterMenu();

    return;
  }


  // ===================================================
  // DELETE
  // ===================================================

  if (
    inButton(
      btnBack,
      tx,
      ty
    )
  ) {

    Serial.println(
      "DELETE PRESSED"
    );


    if (
      inputText.length() > 0
    ) {

      inputText.remove(
        inputText.length() - 1
      );
    }


    calculated = false;
    inputError = false;


    drawInputBox();

    drawButton(
      btnConfirm
    );


    return;
  }


  // ===================================================
  // CALCULATE
  // ===================================================

  if (
    inButton(
      btnEnter,
      tx,
      ty
    )
  ) {

    Serial.println(
      "CALCULATE PRESSED"
    );


    if (
      inputText.length() == 0
    ) {

      Serial.println(
        "NO INPUT"
      );

      return;
    }


    float value =
      inputText.toFloat();


    // INVALID
    if (
      value < 300.0 ||
      value > 500.0
    ) {

      inputError = true;

      calculated = false;


      refreshCal();


      Serial.println(
        "ERROR: 300-500 g"
      );


      return;
    }


    // CALCULATE
    inputError = false;


    calculateSoap();


    calculated = true;


    refreshCal();


    Serial.println(
      "=================="
    );

    Serial.print(
      "SOAP = "
    );

    Serial.println(
      soapValue
    );


    Serial.print(
      "OIL = "
    );

    Serial.println(
      oilValue
    );

    Serial.print("NAOH SOLUTION 30% = ");
    Serial.print(naohSolutionValue, 1);
    Serial.println(" g");

    Serial.println(
      "=================="
    );


    return;
  }


  // ===================================================
  // OK
  // ===================================================

  if (
    inButton(
      btnConfirm,
      tx,
      ty
    )
  ) {

    if (!calculated) {

      Serial.println(
        "PLEASE CALCULATE FIRST"
      );

      return;
    }


    Serial.println(
      "OK PRESSED"
    );

    Serial.println("CAL -> PROCESSING");
    enterProcessing();

    return;
  }
}

// =====================================================
// MODE SETTINGS
// =====================================================

void enterSettings() {

  currentMode =
    MODE_SETTINGS;

  Serial.println(
    "ENTERING MODE SETTINGS"
  );

  // Đảm bảo actuator OFF
  setPumpOil(false);
  setPumpNaOH(false);
  setMixer(false);

  tft.fillScreen(
    BG_COLOR
  );


  // TITLE
  tft.setTextDatum(
    MC_DATUM
  );

  tft.setTextColor(
    TFT_BLACK,
    BG_COLOR
  );

  tft.drawString(
    "SETTINGS",
    240,
    45,
    4
  );


  // BUTTONS
  drawButton(
    btnTimerCalibration
  );

  drawButton(
    btnSettingsBack
  );
}

// =====================================================
// SETTINGS LOOP
// =====================================================

void settingsLoop(bool justPressed) {

  if (!justPressed)
    return;


  // TIMER CALIBRATION
  if (
    inButton(
      btnTimerCalibration,
      tx,
      ty
    )
  ) {

    Serial.println(
      "TIMER CALIBRATION PRESSED"
    );

    enterTimerCalibration();

    return;
  }


  // BACK
  if (
    inButton(
      btnSettingsBack,
      tx,
      ty
    )
  ) {

    Serial.println(
      "SETTINGS BACK"
    );

    enterMenu();

    return;
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);
  // =====================================================
  // MP3 PLAYER
  // =====================================================

  //=====DEFAULT OFF=====
  pinMode(PUMPOIL_PIN, OUTPUT);
  pinMode(PUMPNAOH_PIN, OUTPUT);
  pinMode(MIXER_PIN, OUTPUT);

  digitalWrite(PUMPOIL_PIN, LOW);
  digitalWrite(PUMPNAOH_PIN, LOW);
  temperatureSensor.begin();

  // Không để requestTemperatures() block ~750ms
  temperatureSensor.setWaitForConversion(false);

  Serial.println("DS18B20 READY");

  // Mixer H-Bridge:
  // HIGH = OFF
  // LOW  = ON
  digitalWrite(MIXER_PIN, HIGH);

  Serial.println("PUMP OIL: OFF");
  Serial.println("PUMP NAOH: OFF");


  mp3Serial.begin(
  9600,
  SERIAL_8N1,
  18,   // ESP32 RX <- DFPlayer TX
  17    // ESP32 TX -> DFPlayer RX
  );

  if (mp3.begin(mp3Serial)) {

  Serial.println("DFPlayer OK");

  mp3.volume(25);

  mp3Ready = true;

  } else {

  Serial.println("DFPlayer FAIL");

  mp3Ready = false;
  }

  delay(1000);


  // TFT
  tft.init();

  tft.setRotation(1);

  tft.setTextFont(2);

  tft.setTextPadding(0);


  // Calibration đã test OK
  uint16_t calData[5] = {
    276,
    3589,
    253,
    3547,
    7
  };

  tft.setTouch(
    calData
  );


  setupButtons();


  // BOOT
  enterIntro();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  bool pressed =
    tft.getTouch(
      &tx,
      &ty
    );


  // Cơ chế touch đã test thành công
  bool justPressed =
    pressed &&
    !lastPressed;


  // ===================================================
  // MODE 1
  // ===================================================

  if (
    currentMode ==
    MODE_INTRO
  ) {

    introLoop();
  }


  // ===================================================
  // MODE 2
  // ===================================================

  else if (
    currentMode ==
    MODE_MENU
  ) {

    menuLoop(
      justPressed
    );
  }


  // ===================================================
  // MODE 3
  // ===================================================

  else if (
    currentMode ==
    MODE_CAL
  ) {

    calLoop(
      justPressed
    );
  }

  else if (
  currentMode ==
  MODE_PROCESSING
  ) {

  processingLoop(
    justPressed
  );
  }
  
  else if (
  currentMode ==
  MODE_SETTINGS
  ) {

  settingsLoop(
    justPressed
  );
  }

  else if (
  currentMode ==
  MODE_TIMER_CALIBRATION
  ) {

  timerCalibrationLoop();
  }

  // Save touch state
  lastPressed =
    pressed;


  delay(10);
}

// =====================================================
// DRAW TIMER CALIBRATION SCREEN
// =====================================================

void drawTimerCalibrationScreen(
  String status
) {

  tft.fillScreen(
    BG_COLOR
  );


  tft.setTextDatum(
    MC_DATUM
  );


  tft.setTextColor(
    TFT_BLACK,
    BG_COLOR
  );


  tft.drawString(
    "TIMER CALIBRATION",
    240,
    45,
    4
  );


  tft.setTextColor(
    TFT_BLUE,
    BG_COLOR
  );


  tft.drawString(
    status,
    240,
    150,
    4
  );
}


// =====================================================
// ENTER TIMER CALIBRATION
// =====================================================

void enterTimerCalibration() {

  currentMode =
    MODE_TIMER_CALIBRATION;


  Serial.println();
  Serial.println(
    "============================"
  );

  Serial.println(
    "TIMER CALIBRATION START"
  );

  Serial.println(
    "============================"
  );


  // ===================================================
  // SAFE INITIAL STATE
  // ===================================================

  setPumpOil(false);
  setPumpNaOH(false);
  setMixer(false);


  // ===================================================
  // FIRST WAIT = 5 SECONDS
  // ===================================================

  timerCalibrationState =
    TC_WAIT_START;

  timerCalibrationTimer =
    millis();


  drawTimerCalibrationScreen(
    "Starting in 5 seconds..."
  );


  Serial.println(
    "Wait 5 seconds"
  );
}


// =====================================================
// TIMER CALIBRATION LOOP
// =====================================================

void timerCalibrationLoop() {

  unsigned long now =
    millis();


  // ===================================================
  // STATE 1
  // WAIT 5 SECONDS
  // ===================================================

  if (
    timerCalibrationState ==
    TC_WAIT_START
  ) {

    if (
      now - timerCalibrationTimer >= 5000
    ) {

      // OIL ON
      setPumpOil(true);


      Serial.println(
        "CALIBRATION: OIL ON"
      );

      Serial.println(
        "Oil will run for 10 seconds"
      );


      drawTimerCalibrationScreen(
        "Pump Oil: ON"
      );


      timerCalibrationState =
        TC_OIL_RUNNING;

      timerCalibrationTimer =
        now;
    }
  }


  // ===================================================
  // STATE 2
  // OIL RUN 10 SECONDS
  // ===================================================

  else if (
    timerCalibrationState ==
    TC_OIL_RUNNING
  ) {

    if (
      now - timerCalibrationTimer >= 10000
    ) {

      // TẮT OIL NGAY
      setPumpOil(false);


      Serial.println(
        "CALIBRATION: OIL OFF"
      );

      Serial.println(
        "Wait 30 seconds before NaOH"
      );


      drawTimerCalibrationScreen(
        "Wait 30 seconds..."
      );


      timerCalibrationState =
        TC_WAIT_NAOH;

      timerCalibrationTimer =
        now;
    }
  }


  // ===================================================
  // STATE 3
  // WAIT 10 SECONDS
  // ===================================================

  else if (
    timerCalibrationState ==
    TC_WAIT_NAOH
  ) {

    if (
      now - timerCalibrationTimer >= 10000
    ) {

      // NAOH ON
      setPumpNaOH(true);


      Serial.println(
        "CALIBRATION: NAOH ON"
      );

      Serial.println(
        "NaOH will run for 10 seconds"
      );


      drawTimerCalibrationScreen(
        "Pump NaOH: ON"
      );


      timerCalibrationState =
        TC_NAOH_RUNNING;

      timerCalibrationTimer =
        now;
    }
  }


  // ===================================================
  // STATE 4
  // NAOH RUN 10 SECONDS
  // ===================================================

  else if (
    timerCalibrationState ==
    TC_NAOH_RUNNING
  ) {

    if (
      now - timerCalibrationTimer >= 10000
    ) {

      // TẮT NAOH NGAY
      setPumpNaOH(false);


      Serial.println(
        "CALIBRATION: NAOH OFF"
      );


      Serial.println(
        "TIMER CALIBRATION COMPLETE"
      );

      Serial.println(
        "RETURN TO SETTINGS"
      );


      // tự động quay lại SETTINGS
      enterSettings();

      return;
    }
  }
}