/* =====================================================================
   AUTOSOAP CPM — FIRMWARE ESP32-S3 (Giai đoạn C)
   Board: ESP32-S3-N16R8 (Arduino IDE, board "ESP32S3 Dev Module")

   CHỨC NĂNG:
   - Đọc DS18B20 (nhiệt độ buồng phản ứng) + HX711/Loadcell (khối lượng)
   - Điều khiển: 2 bơm màng (nước/dầu), motor bước (van NaOH), mixer (L9110S),
     PTC heater (qua relay)
   - Gửi telemetry lên Supabase mỗi TELEMETRY_INTERVAL_MS qua hàm RPC
     device_push_telemetry (không cần MQTT/gateway riêng)
   - Lấy lệnh (START_BATCH/PAUSE/RESUME/SOS/MAINTENANCE) qua hàm RPC
     device_pull_commands, thực thi, rồi báo lại qua device_ack_command
   - KHÔNG thay thế mạch an toàn vật lý: E-stop/quá nhiệt/giới hạn bồn PHẢI
     là mạch cứng độc lập, cắt nguồn trực tiếp không qua ESP32 (xem SOP mục 1)

   THƯ VIỆN CẦN CÀI (Arduino IDE > Library Manager, đều miễn phí):
   - ArduinoJson (Benoit Blanchon)
   - OneWire
   - DallasTemperature
   - HX711 (bogde/HX711 hoặc tương đương)
   - ESP32 board package (Espressif) — cài qua Boards Manager
   ===================================================================== */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HX711.h>

// ---------------------------------------------------------------------
// 1. CẤU HÌNH — SỬA CÁC GIÁ TRỊ NÀY CHO ĐÚNG MÁY CỦA BẠN
// ---------------------------------------------------------------------
const char* WIFI_SSID     = "TEN_WIFI_CUA_BAN";
const char* WIFI_PASSWORD = "MAT_KHAU_WIFI";

// Lấy 2 giá trị dưới từ Supabase Project Settings > API
const char* SUPABASE_URL      = "https://ipyaetrnmcjdgahjpbdz.supabase.co";
const char* SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImlweWFldHJubWNqZGdhaGpwYmR6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODczOTAzNjEsImV4cCI6MjEwMjk2NjM2MX0.g9NSZwBr8Xm7ii8JW3-ydoRs1dOeS-vUVDCgUDdqE1Q";

// Mã máy + device secret riêng (đã được cấp sẵn cho AS-CPM-001)
const char* MACHINE_CODE   = "AS-CPM-001";
const char* DEVICE_SECRET  = "ff54c7df52cafc17666a4470716b5fc11f33646c652ea8d8";

const unsigned long TELEMETRY_INTERVAL_MS = 3000;   // gửi telemetry mỗi 3 giây
const unsigned long COMMAND_POLL_MS       = 2000;   // hỏi lệnh mới mỗi 2 giây

// ---------------------------------------------------------------------
// 2. SƠ ĐỒ CHÂN (GPIO) — SỬA THEO SƠ ĐỒ NGUYÊN LÝ THỰC TẾ CỦA BẠN
// ---------------------------------------------------------------------
#define PIN_DS18B20        4     // OneWire cảm biến nhiệt độ
#define PIN_HX711_DOUT      16
#define PIN_HX711_SCK       17

#define PIN_RELAY_PUMP_WATER 5   // Relay bơm nước
#define PIN_RELAY_PUMP_OIL   18  // Relay bơm dầu
#define PIN_RELAY_HEATER     19  // Relay PTC heater (chỉ dùng khi chạy mỡ động vật)

#define PIN_STEPPER_STEP     21  // Motor bước mở van NaOH (driver rời, vd A4988/ULN2003)
#define PIN_STEPPER_DIR      22
#define PIN_STEPPER_ENABLE   23

#define PIN_MIXER_IN1        25  // L9110S điều khiển mixer DC
#define PIN_MIXER_IN2        26

#define PIN_SOS_BUZZER_LED   27  // LED/còi báo SOS cục bộ (KHÔNG thay thế E-stop vật lý)

// ---------------------------------------------------------------------
// 3. ĐỐI TƯỢNG CẢM BIẾN
// ---------------------------------------------------------------------
OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&oneWire);
HX711 scale;

// Hệ số hiệu chuẩn Loadcell — PHẢI hiệu chuẩn lại bằng quả tạ chuẩn 500g
// theo đúng quy trình D2 trong báo cáo trước khi tin số liệu
float HX711_CALIBRATION_FACTOR = 420.0;

// ---------------------------------------------------------------------
// 4. TRẠNG THÁI MÁY (đồng bộ với UI web qua các cột telemetry)
// ---------------------------------------------------------------------
bool stateHeater = false;
bool statePumpWater = false;
bool statePumpOil = false;
bool stateMixer = false;
bool stateNaohValve = false;
bool emergencyStopped = false;   // set true khi nhận SOS qua backend (không thay E-stop vật lý)

unsigned long lastTelemetryMs = 0;
unsigned long lastCommandPollMs = 0;

// =======================================================================
// SETUP
// =======================================================================
void setup() {
  Serial.begin(115200);

  pinMode(PIN_RELAY_PUMP_WATER, OUTPUT);
  pinMode(PIN_RELAY_PUMP_OIL, OUTPUT);
  pinMode(PIN_RELAY_HEATER, OUTPUT);
  pinMode(PIN_STEPPER_STEP, OUTPUT);
  pinMode(PIN_STEPPER_DIR, OUTPUT);
  pinMode(PIN_STEPPER_ENABLE, OUTPUT);
  pinMode(PIN_MIXER_IN1, OUTPUT);
  pinMode(PIN_MIXER_IN2, OUTPUT);
  pinMode(PIN_SOS_BUZZER_LED, OUTPUT);
  allActuatorsOff(); // fail-safe: khởi động luôn ở trạng thái an toàn

  ds18b20.begin();
  scale.begin(PIN_HX711_DOUT, PIN_HX711_SCK);
  scale.set_scale(HX711_CALIBRATION_FACTOR);
  scale.tare(); // Loadcell về 0 khi chưa có gì trên cân

  connectWiFi();
}

// =======================================================================
// LOOP
// =======================================================================
void loop() {
  // FAIL-SAFE: mất WiFi -> tắt hết cơ cấu chấp hành, không đoán mò tiếp tục chạy
  if (WiFi.status() != WL_CONNECTED) {
    allActuatorsOff();
    connectWiFi();
    return;
  }

  unsigned long now = millis();

  if (now - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryMs = now;
    sendTelemetry();
  }

  if (now - lastCommandPollMs >= COMMAND_POLL_MS) {
    lastCommandPollMs = now;
    pollAndExecuteCommands();
  }
}

// =======================================================================
// KẾT NỐI WIFI
// =======================================================================
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Đang kết nối WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " OK" : " THẤT BẠI (sẽ thử lại)");
}

// =======================================================================
// GỬI TELEMETRY LÊN SUPABASE (RPC device_push_telemetry)
// =======================================================================
void sendTelemetry() {
  ds18b20.requestTemperatures();
  float temperature = ds18b20.getTempCByIndex(0); // -127 nếu lỗi cảm biến
  float massGram = scale.is_ready() ? scale.get_units(5) : -1;

  // TODO: thay 3 dòng dưới bằng % mức bồn thật khi có cảm biến mức bồn riêng.
  // Hiện dùng tạm khối lượng cân được làm tín hiệu tham khảo.
  float tankOilPct = -1;   // -1 nghĩa là "chưa có dữ liệu", web sẽ tự bỏ qua field null
  float tankWaterPct = -1;
  float tankNaohPct = -1;

  String alarm = "";
  if (temperature > -50 && temperature < -10) alarm = "Lỗi cảm biến DS18B20";
  if (temperature >= 65) alarm = "CẢNH BÁO: Nhiệt độ vượt ngưỡng an toàn";

  StaticJsonDocument<512> body;
  body["p_machine_code"] = MACHINE_CODE;
  body["p_device_secret"] = DEVICE_SECRET;
  if (temperature > -50) body["p_temperature"] = temperature;
  if (tankOilPct >= 0) body["p_tank_oil"] = tankOilPct;
  if (tankWaterPct >= 0) body["p_tank_water"] = tankWaterPct;
  if (tankNaohPct >= 0) body["p_tank_naoh"] = tankNaohPct;
  body["p_pump_oil"] = statePumpOil;
  body["p_pump_water"] = statePumpWater;
  body["p_naoh_valve"] = stateNaohValve;
  body["p_mixer"] = stateMixer;
  body["p_heater"] = stateHeater;
  if (alarm.length() > 0) body["p_alarm"] = alarm;

  String payload;
  serializeJson(body, payload);
  callSupabaseRpc("device_push_telemetry", payload);
}

// =======================================================================
// LẤY LỆNH ĐANG CHỜ VÀ THỰC THI (RPC device_pull_commands)
// =======================================================================
void pollAndExecuteCommands() {
  StaticJsonDocument<256> body;
  body["p_machine_code"] = MACHINE_CODE;
  body["p_device_secret"] = DEVICE_SECRET;
  String payload;
  serializeJson(body, payload);

  String response = callSupabaseRpc("device_pull_commands", payload);
  if (response.length() == 0) return;

  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) { Serial.println("Lỗi parse JSON lệnh"); return; }

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject cmd : arr) {
    String requestId = cmd["request_id"].as<String>();
    String command = cmd["command"].as<String>();
    Serial.println("Nhận lệnh: " + command + " (requestId=" + requestId + ")");

    executeCommand(command, cmd);

    ackCommand(requestId, "COMPLETED");
  }
}

// =======================================================================
// THỰC THI LỆNH THEO KỊCH BẢN — sửa lại đúng logic quy trình thật của bạn
// =======================================================================
void executeCommand(String command, JsonObject payload) {
  if (command == "SOS") {
    emergencyStopped = true;
    allActuatorsOff();
    digitalWrite(PIN_SOS_BUZZER_LED, HIGH);
    Serial.println(">>> SOS: đã tắt toàn bộ cơ cấu chấp hành qua phần mềm.");
    Serial.println(">>> LƯU Ý: mạch E-stop vật lý mới là lớp an toàn chính.");
  }
  else if (command == "MAINTENANCE") {
    allActuatorsOff();
  }
  else if (command == "RESUME") {
    emergencyStopped = false;
    digitalWrite(PIN_SOS_BUZZER_LED, LOW);
  }
  else if (command == "START_BATCH") {
    if (emergencyStopped) { Serial.println("Đang SOS, bỏ qua lệnh START_BATCH"); return; }
    // TODO: cắm state machine thật của quy trình (bơm nước -> mở van NaOH ->
    // bơm dầu -> khuấy đến "trạng thái vệt" -> dừng) theo đúng mục C2 báo cáo.
    Serial.println("Bắt đầu mẻ mới (placeholder — cần cắm state machine thật)");
  }
  else if (command == "PAUSE") {
    setMixer(false); setPumpWater(false); setPumpOil(false); setHeater(false);
  }
}

// =======================================================================
// BÁO KẾT QUẢ LỆNH VỀ SUPABASE (RPC device_ack_command)
// =======================================================================
void ackCommand(String requestId, String status) {
  StaticJsonDocument<256> body;
  body["p_machine_code"] = MACHINE_CODE;
  body["p_device_secret"] = DEVICE_SECRET;
  body["p_request_id"] = requestId;
  body["p_status"] = status;
  String payload;
  serializeJson(body, payload);
  callSupabaseRpc("device_ack_command", payload);
}

// =======================================================================
// HÀM DÙNG CHUNG: GỌI 1 RPC BẤT KỲ TRÊN SUPABASE
// =======================================================================
String callSupabaseRpc(const char* functionName, String jsonBody) {
  WiFiClientSecure client;
  client.setInsecure(); // đơn giản hoá cho ESP32; có thể thay bằng cert thật nếu muốn chặt hơn

  HTTPClient https;
  String url = String(SUPABASE_URL) + "/rest/v1/rpc/" + functionName;

  if (!https.begin(client, url)) {
    Serial.println("Không mở được kết nối HTTPS tới Supabase");
    return "";
  }
  https.addHeader("Content-Type", "application/json");
  https.addHeader("apikey", SUPABASE_ANON_KEY);
  https.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);

  int httpCode = https.POST(jsonBody);
  String response = "";
  if (httpCode == 200 || httpCode == 201) {
    response = https.getString();
  } else {
    Serial.printf("Supabase RPC %s trả về mã lỗi: %d\n", functionName, httpCode);
    Serial.println(https.getString());
  }
  https.end();
  return response;
}

// =======================================================================
// ĐIỀU KHIỂN CƠ CẤU CHẤP HÀNH
// =======================================================================
void setPumpWater(bool on) { statePumpWater = on; digitalWrite(PIN_RELAY_PUMP_WATER, on ? HIGH : LOW); }
void setPumpOil(bool on)   { statePumpOil = on;   digitalWrite(PIN_RELAY_PUMP_OIL, on ? HIGH : LOW); }
void setHeater(bool on)    { stateHeater = on;    digitalWrite(PIN_RELAY_HEATER, on ? HIGH : LOW); }
void setMixer(bool on) {
  stateMixer = on;
  digitalWrite(PIN_MIXER_IN1, on ? HIGH : LOW);
  digitalWrite(PIN_MIXER_IN2, LOW);
}

void allActuatorsOff() {
  setPumpWater(false);
  setPumpOil(false);
  setHeater(false);
  setMixer(false);
  digitalWrite(PIN_STEPPER_ENABLE, HIGH); // HIGH thường = disable driver (tuỳ loại driver)
  stateNaohValve = false;
}
