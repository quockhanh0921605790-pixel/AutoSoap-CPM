enum ProcessingState {
  PROC_MUSIC5_WAIT,
  PROC_OIL_ON,
  PROC_WAIT_AFTER_OIL,
  PROC_MIXER_WAIT,
  PROC_NAOH_ON,
  PROC_MIXING,
  PROC_FINISH_WAIT
};

ProcessingState processingState = PROC_MUSIC5_WAIT;

unsigned long processingTimer = 0;

// =====================================================
// PROCESSING STATE
// =====================================================

bool pumpOilState  = false;
bool pumpNaOHState = false;
bool mixerState    = false;


// Trạng thái lần trước để TFT chỉ update khi có thay đổi
bool oldPumpOilState  = false;
bool oldPumpNaOHState = false;
bool oldMixerState    = false;

bool firstStepDraw = true;

//====STOP====
Button btnStop;

// =====================================================
// TEMPERATURE
// =====================================================

float processingTemperature = 0.0;

unsigned long temperatureTimer = 0;

bool temperatureRequestPending = false;
unsigned long temperatureRequestTime = 0;

const unsigned long TEMP_UPDATE_INTERVAL = 1000;
const unsigned long TEMP_CONVERSION_TIME = 750;

// =====================================================
// PUMP FLOW CALIBRATION
// Đơn vị: gram / second (g/s)
// =====================================================

// XXX = thay bằng giá trị thực nghiệm sau khi calibrate
float OIL_FLOW_RATE  = XXX;   // g/s
float NAOH_FLOW_RATE = XXX;   // g/s

unsigned long oilPumpTime  = 0;
unsigned long naohPumpTime = 0;

// =====================================================
// FUNCTION PROTOTYPES
// =====================================================

void drawProcessingUI();
void drawTargetColumn();
void drawStepColumn();

void updateStepColumn();

void setPumpOil(bool state);
void setPumpNaOH(bool state);
void setMixer(bool state);


// =====================================================
// ENTER PROCESSING
// =====================================================

void enterProcessing() {

  currentMode = MODE_PROCESSING;

  Serial.println();
  Serial.println("============================");
  Serial.println("ENTERING MODE PROCESSING");
  Serial.println("============================");

  calculatePumpTimes();

  // ===================================================
  // SAFE INITIAL STATE
  // ===================================================

  setPumpOil(false);
  setPumpNaOH(false);
  setMixer(false);


  firstStepDraw = true;


  // ===================================================
  // STOP BUTTON
  // ===================================================

  btnStop = {
    35, 255,
    170, 50,
    "STOP"
  };


  // ===================================================
  // TARGET INFO
  // ===================================================

  Serial.print("Waste Oil Target: ");
  Serial.print(oilValue, 1);
  Serial.println(" g");

  Serial.print("NaOH Solution 30% Target: ");
  Serial.print(naohSolutionValue, 1);
  Serial.println(" g");


  // ===================================================
  // DRAW UI
  // ===================================================

  drawProcessingUI();

  // ===================================================
  // TEMPERATURE START
  // ===================================================

  temperatureRequestPending = false;

  // Cho phép request ngay
  temperatureTimer =
  millis() - TEMP_UPDATE_INTERVAL;

  processingTemperature = 0.0;

  // ===================================================
  // START PROCESSING SEQUENCE
  // ===================================================

  Serial.println();
  Serial.println("PROCESSING SEQUENCE START");


  // Music file 5
  playModeMusic(4);

  Serial.println("Music 5");
  Serial.println("Wait 5 seconds");


  processingState =
    PROC_MUSIC5_WAIT;

  processingTimer =
    millis();
}


// =====================================================
// MAIN PROCESSING UI
// =====================================================

void drawProcessingUI() {

  tft.fillScreen(BG_COLOR);


  // Vertical divider
  tft.drawFastVLine(
    240,
    0,
    320,
    BORDER_COLOR
  );


  // TARGET
  drawTargetColumn();

  // ===================================================
  // STOP BUTTON
  // ===================================================

  tft.fillRoundRect(
  btnStop.x,
  btnStop.y,
  btnStop.w,
  btnStop.h,
  8,
  TFT_RED
  );

  tft.drawRoundRect(
  btnStop.x,
  btnStop.y,
  btnStop.w,
  btnStop.h,
  8,
  TFT_DARKGREY
  );

  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
  TFT_WHITE,
  TFT_RED
  );

  tft.drawString(
  "STOP",
  btnStop.x + btnStop.w / 2,
  btnStop.y + btnStop.h / 2,
  4
  );

  // STEP
  drawStepColumn();
}


// =====================================================
// TARGET COLUMN
// LEFT SIDE
//
// CONSTANT — chỉ vẽ khi vào Processing
// =====================================================

void drawTargetColumn() {

  // Header
  tft.fillRoundRect(
    10,
    10,
    220,
    45,
    8,
    TFT_CYAN
  );


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_BLACK,
    TFT_CYAN
  );


  tft.drawString(
    "TARGET",
    120,
    32,
    4
  );


  // ===================================================
  // OIL TARGET
  // ===================================================

  tft.setTextDatum(TL_DATUM);


  tft.setTextColor(
    TFT_DARKGREEN,
    BG_COLOR
  );


  tft.drawString(
    "Waste Oil",
    20,
    80,
    2
  );


  tft.drawString(
    String(oilValue, 1) + " g",
    20,
    105,
    4
  );


  // ===================================================
  // NAOH SOLUTION TARGET
  // ===================================================

  tft.setTextColor(
    TFT_RED,
    BG_COLOR
  );


  tft.drawString(
    "NaOH Solution 30%",
    20,
    165,
    2
  );


  tft.drawString(
    String(naohSolutionValue, 1) + " g",
    20,
    190,
    4
  );
}


// =====================================================
// STEP COLUMN
// RIGHT SIDE
// =====================================================

void drawStepColumn() {

  // Header
  tft.fillRoundRect(
    250,
    10,
    220,
    45,
    8,
    TFT_YELLOW
  );


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_BLACK,
    TFT_YELLOW
  );


  tft.drawString(
    "STEP",
    360,
    32,
    4
  );


  // ===================================================
  // LABELS
  // ===================================================

  tft.setTextDatum(TL_DATUM);


  // Pump Oil
  tft.setTextColor(
    TFT_BLACK,
    BG_COLOR
  );

  tft.drawString(
    "Pump Oil:",
    260,
    80,
    2
  );


  // Pump NaOH
  tft.drawString(
    "Pump NaOH:",
    260,
    150,
    2
  );


  // Mixer
  tft.drawString(
    "Mixer Motor:",
    260,
    220,
    2
  );

  // ===================================================
  // TEMPERATURE
  // ===================================================

  tft.setTextDatum(TL_DATUM);

  tft.setTextColor(
  TFT_BLACK,
  BG_COLOR
  );

  tft.drawString(
  "Temperature:",
  260,
  270,
  2
  );

  // Force status draw
  firstStepDraw = true;

  updateStepColumn();
}


// =====================================================
// DRAW ON/OFF STATUS
// =====================================================

void drawStatus(
  int x,
  int y,
  bool state
) {

  uint16_t color;

  String text;


  if (state) {

    color = TFT_GREEN;
    text = "ON";

  } else {

    color = TFT_RED;
    text = "OFF";
  }


  // Clear previous status
  tft.fillRoundRect(
    x,
    y,
    80,
    35,
    6,
    color
  );


  tft.setTextDatum(MC_DATUM);

  tft.setTextColor(
    TFT_WHITE,
    color
  );


  tft.drawString(
    text,
    x + 40,
    y + 17,
    2
  );
}


// =====================================================
// UPDATE STEP COLUMN
//
// Chỉ redraw status nào thay đổi.
// TARGET hoàn toàn không bị redraw.
// =====================================================

void updateStepColumn() {


  // ===================================================
  // PUMP OIL
  // ===================================================

  if (
    firstStepDraw ||
    pumpOilState != oldPumpOilState
  ) {

    drawStatus(
      370,
      70,
      pumpOilState
    );


    oldPumpOilState =
      pumpOilState;
  }


  // ===================================================
  // PUMP NAOH
  // ===================================================

  if (
    firstStepDraw ||
    pumpNaOHState != oldPumpNaOHState
  ) {

    drawStatus(
      370,
      140,
      pumpNaOHState
    );


    oldPumpNaOHState =
      pumpNaOHState;
  }


  // ===================================================
  // MIXER
  // ===================================================

  if (
    firstStepDraw ||
    mixerState != oldMixerState
  ) {

    drawStatus(
      370,
      210,
      mixerState
    );


    oldMixerState =
      mixerState;
  }


  firstStepDraw = false;
}


// =====================================================
// PUMP OIL CONTROL
// =====================================================

void setPumpOil(bool state) {

  pumpOilState = state;


  if (state) {

    digitalWrite(
      PUMPOIL_PIN,
      HIGH
    );

    Serial.println(
      "Pump Oil: ON"
    );

  } else {

    digitalWrite(
      PUMPOIL_PIN,
      LOW
    );

    Serial.println(
      "Pump Oil: OFF"
    );
  }
}


// =====================================================
// PUMP NAOH CONTROL
// =====================================================

void setPumpNaOH(bool state) {

  pumpNaOHState = state;


  if (state) {

    digitalWrite(
      PUMPNAOH_PIN,
      HIGH
    );

    Serial.println(
      "Pump NaOH: ON"
    );

  } else {

    digitalWrite(
      PUMPNAOH_PIN,
      LOW
    );

    Serial.println(
      "Pump NaOH: OFF"
    );
  }
}


// =====================================================
// MIXER CONTROL
// GPIO21 H-BRIDGE
// =====================================================

void setMixer(bool state) {

  mixerState = state;


  if (state) {

    digitalWrite(
      MIXER_PIN,
      LOW
    );

    Serial.println(
      "Mixer Motor: ON"
    );

  } else {

    digitalWrite(
      MIXER_PIN,
      HIGH
    );

    Serial.println(
      "Mixer Motor: OFF"
    );
  }
}


// =====================================================
// PROCESSING LOOP
// =====================================================

void processingLoop(bool justPressed) {

  // ===================================================
  // UPDATE STEP UI
  // ===================================================

  updateStepColumn();
  updateProcessingTemperature();

  // ===================================================
  // STOP BUTTON
  // LUÔN được kiểm tra trước sequence
  // ===================================================

  if (justPressed) {

    if (
      inButton(
        btnStop,
        tx,
        ty
      )
    ) {

      Serial.println();
      Serial.println("============================");
      Serial.println("STOP PRESSED");
      Serial.println("============================");


      // ===============================================
      // ALL MODULES OFF
      // ===============================================

      setPumpOil(false);
      setPumpNaOH(false);
      setMixer(false);


      Serial.println("ALL MODULES OFF");
      Serial.println("PROCESSING ABORTED");
      Serial.println("PROCESSING -> MENU");


      enterMenu();

      return;
    }
  }


  // ===================================================
  // CURRENT TIME
  // ===================================================

  unsigned long now =
    millis();


  // ===================================================
  // STATE 1
  //
  // Music 5 đã phát khi enterProcessing()
  // Chờ 5 giây
  // ===================================================

  if (
    processingState ==
    PROC_MUSIC5_WAIT
  ) {

    if (
      now - processingTimer >= 5000
    ) {

      Serial.println();
      Serial.println("5s finished");


      // OIL ON
      setPumpOil(true);


      processingState =
        PROC_OIL_ON;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 2
  //
  // OIL ON trong 5 giây
  // ===================================================

  else if (
    processingState ==
    PROC_OIL_ON
  ) {

    if (
      now - processingTimer >= oilPumpTime
    ) {

      // OIL OFF
      setPumpOil(false);


      Serial.println(
        "Wait 2 seconds"
      );


      processingState =
        PROC_WAIT_AFTER_OIL;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 3
  //
  // Oil OFF
  // Chờ 2 giây
  // ===================================================

  else if (
    processingState ==
    PROC_WAIT_AFTER_OIL
  ) {

    if (
      now - processingTimer >= 2000
    ) {

      // MIXER ON
      setMixer(true);


      Serial.println(
        "Mixer warm-up: 1 second"
      );


      processingState =
        PROC_MIXER_WAIT;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 4
  //
  // Mixer ON
  // Chờ 1 giây trước NaOH
  // ===================================================

  else if (
    processingState ==
    PROC_MIXER_WAIT
  ) {

    if (
      now - processingTimer >= 1000
    ) {

      // NaOH Solution Pump ON
      setPumpNaOH(true);


      processingState =
        PROC_NAOH_ON;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 5
  //
  // NaOH Solution Pump ON
  // trong 3 giây
  //
  // Mixer vẫn ON
  // ===================================================

  else if (
    processingState ==
    PROC_NAOH_ON
  ) {

    if (
      now - processingTimer >= naohPumpTime
    ) {

      // NaOH Pump OFF
      setPumpNaOH(false);


      Serial.println(
        "NaOH pump finished"
      );

      Serial.println(
        "Continue mixing: 10 seconds"
      );


      processingState =
        PROC_MIXING;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 6
  //
  // NaOH OFF
  // Mixer tiếp tục ON 10 giây
  // ===================================================

  else if (
    processingState ==
    PROC_MIXING
  ) {

    if (
      now - processingTimer >= 10000
    ) {

      // MIXER OFF
      setMixer(false);


      Serial.println();
      Serial.println(
        "PROCESSING COMPLETE"
      );


      // Music file 6
      playModeMusic(5);

      Serial.println(
        "Music 6"
      );

      Serial.println(
        "Wait 2 seconds"
      );


      processingState =
        PROC_FINISH_WAIT;

      processingTimer =
        now;
    }
  }


  // ===================================================
  // STATE 7
  //
  // Music 6
  // Chờ 2 giây rồi MENU
  // ===================================================

  else if (
    processingState ==
    PROC_FINISH_WAIT
  ) {

    if (
      now - processingTimer >= 2000
    ) {

      // Safety check trước khi rời Processing
      setPumpOil(false);
      setPumpNaOH(false);
      setMixer(false);


      Serial.println();
      Serial.println(
        "PROCESSING -> MENU"
      );


      enterMenu();

      return;
    }
  }
}

// =====================================================
// DRAW TEMPERATURE
// =====================================================

void drawProcessingTemperature() {

  // Xóa riêng vùng số nhiệt độ
  tft.fillRect(
    370,
    265,
    95,
    35,
    BG_COLOR
  );


  tft.setTextDatum(MC_DATUM);


  // Sensor error
  if (
    processingTemperature <= -100.0
  ) {

    tft.setTextColor(
      TFT_RED,
      BG_COLOR
    );

    tft.drawString(
      "ERROR",
      415,
      282,
      2
    );

    return;
  }


  // Normal
  tft.setTextColor(
    TFT_BLUE,
    BG_COLOR
  );


  String tempText =
    String(processingTemperature, 1) +
    " C";


  tft.drawString(
    tempText,
    415,
    282,
    2
  );
}

void updateProcessingTemperature() {

  unsigned long now = millis();


  // ===================================================
  // START NEW CONVERSION
  // ===================================================

  if (!temperatureRequestPending) {

    if (
      now - temperatureTimer >=
      TEMP_UPDATE_INTERVAL
    ) {

      temperatureSensor.requestTemperatures();

      temperatureRequestPending = true;

      temperatureRequestTime = now;
    }

    return;
  }


  // ===================================================
  // WAIT FOR SENSOR
  //
  // Không dùng delay(750)
  // ===================================================

  if (
    now - temperatureRequestTime >=
    TEMP_CONVERSION_TIME
  ) {

    float newTemperature =
      temperatureSensor.getTempCByIndex(0);


    temperatureRequestPending = false;

    temperatureTimer = now;


    // =================================================
    // CHECK SENSOR
    // =================================================

    if (
      newTemperature ==
      DEVICE_DISCONNECTED_C
    ) {

      processingTemperature = -127.0;

      Serial.println(
        "DS18B20 ERROR"
      );
    }

    else {

      processingTemperature =
        newTemperature;


      Serial.print(
        "Temperature: "
      );

      Serial.print(
        processingTemperature,
        1
      );

      Serial.println(
        " C"
      );
    }


    // Update riêng nhiệt độ
    drawProcessingTemperature();
  }
}

void calculatePumpTimes() {

  // Oil
  oilPumpTime =
    (unsigned long)((oilValue / OIL_FLOW_RATE) * 1000.0);

  // NaOH Solution 30%
  naohPumpTime =
    (unsigned long)((naohSolutionValue / NAOH_FLOW_RATE) * 1000.0);


  Serial.println("===== PUMP CALCULATION =====");

  Serial.print("Oil target: ");
  Serial.print(oilValue, 1);
  Serial.println(" g");

  Serial.print("Oil flow: ");
  Serial.print(OIL_FLOW_RATE, 2);
  Serial.println(" g/s");

  Serial.print("Oil pump time: ");
  Serial.print(oilPumpTime);
  Serial.println(" ms");


  Serial.print("NaOH Solution target: ");
  Serial.print(naohSolutionValue, 1);
  Serial.println(" g");

  Serial.print("NaOH flow: ");
  Serial.print(NAOH_FLOW_RATE, 2);
  Serial.println(" g/s");

  Serial.print("NaOH pump time: ");
  Serial.print(naohPumpTime);
  Serial.println(" ms");

  Serial.println("============================");
}