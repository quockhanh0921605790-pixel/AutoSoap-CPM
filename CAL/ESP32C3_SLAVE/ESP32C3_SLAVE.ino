//===== CylinderGate.ino (ESP32-C3 SuperMini + A4988 + NEMA17) =====
// Nhận lệnh chiều từ ESP32-S3 (GPIO16) qua SLAVE_CMD
// Mỗi khi mức lệnh THAY ĐỔI (hoặc lần đầu sau boot) -> quay đúng 3 giây rồi dừng
// HIGH = clockwise (mở)   |   LOW = counter-clockwise (đóng)

#define STEP_PIN  0
#define DIR_PIN   1
#define SLAVE_CMD 4   // nối với GPIO16 (CYLINDER_GATE) bên ESP32-S3

const int   STEPS_PER_REV   = 200;    // full-step, NEMA17 1.8°/bước
const float MOTOR_RPM       = 30.0;   // tốc độ quay
const unsigned long SPIN_MS = 3000UL; // quay 3 giây mỗi lần đổi lệnh

unsigned long stepIntervalUs;
unsigned long lastStepTime = 0;

int  lastCmd        = -1;     // -1 = chưa xử lý lần nào -> ép kích hoạt ngay lần đọc đầu tiên
bool spinning        = false;
unsigned long spinStartTime = 0;

void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(STEP_PIN, LOW);

  pinMode(SLAVE_CMD, INPUT_PULLDOWN);

  stepIntervalUs = (unsigned long)(60.0 * 1000000.0 / MOTOR_RPM / STEPS_PER_REV);
  lastStepTime = micros();

  Serial.println("ESP32-C3 Cylinder Gate READY (A4988 + NEMA17)");
}

void loop() {
  int cmd = digitalRead(SLAVE_CMD);

  // Phát hiện lệnh mới (đổi mức HIGH<->LOW, hoặc lần đọc đầu tiên sau boot)
  if (cmd != lastCmd) {
    lastCmd = cmd;
    spinning = true;
    spinStartTime = millis();
    digitalWrite(DIR_PIN, cmd ? LOW : HIGH);   // HIGH=CW(mở), LOW=CCW(đóng)
    Serial.println(cmd ? "Mo cylinder gate (CW) - quay 3s" : "Dong cylinder gate (CCW) - quay 3s");
  }

  if (spinning) {
    if (millis() - spinStartTime >= SPIN_MS) {
      spinning = false;   // hết 3 giây -> dừng, không step nữa
    } else {
      unsigned long now = micros();
      if (now - lastStepTime >= stepIntervalUs) {
        lastStepTime = now;
        digitalWrite(STEP_PIN, HIGH);
        delayMicroseconds(3);
        digitalWrite(STEP_PIN, LOW);
      }
    }
  }
}