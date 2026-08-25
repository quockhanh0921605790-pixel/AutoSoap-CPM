# AutoSoap CPM — Firmware ESP32-S3

Firmware mẫu cho máy AutoSoap CPM (mã máy `AS-CPM-001`), kết nối trực tiếp tới backend Supabase qua HTTPS REST/RPC — không cần MQTT/gateway riêng.

## Cài đặt Arduino IDE (một lần)

1. Cài [Arduino IDE](https://www.arduino.cc/en/software) (miễn phí).
2. `File > Preferences` → ô "Additional Boards Manager URLs" → dán:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. `Tools > Board > Boards Manager` → tìm `esp32` (Espressif Systems) → Install.
4. `Tools > Manage Libraries` → cài các thư viện sau (đều miễn phí):
   - `ArduinoJson` (Benoit Blanchon)
   - `OneWire`
   - `DallasTemperature`
   - `HX711` (bogde)

## Mở project này

- Cách 1 (khuyên dùng): `git clone` repo này về máy, sau đó mở file
  `AutoSoapCPM_Firmware.ino` bằng Arduino IDE (double-click file `.ino`,
  **không mở file .zip hay README**).
- Cách 2: Vào trang GitHub repo → nút xanh **Code > Download ZIP** → giải nén
  → mở file `.ino` bên trong.

## Trước khi nạp vào máy — BẮT BUỘC sửa các giá trị sau trong file `.ino`

| Biến | Ý nghĩa |
|---|---|
| `WIFI_SSID`, `WIFI_PASSWORD` | WiFi thật của xưởng/nhà bạn |
| Mục "2. SƠ ĐỒ CHÂN (GPIO)" | Phải khớp đúng sơ đồ mạch KiCad thật bạn đã vẽ |
| `HX711_CALIBRATION_FACTOR` | Hiệu chuẩn lại bằng quả tạ chuẩn 500g trước khi tin số liệu |

`SUPABASE_URL`, `SUPABASE_ANON_KEY`, `MACHINE_CODE`, `DEVICE_SECRET` đã được
điền sẵn đúng cho máy `AS-CPM-001` — không cần sửa trừ khi bạn tạo máy mới.

## Nạp code

1. Cắm ESP32-S3 qua USB.
2. `Tools > Board` → chọn `ESP32S3 Dev Module`.
3. `Tools > Port` → chọn đúng cổng COM hiện ra.
4. Bấm nút Upload (mũi tên →).
5. Mở `Tools > Serial Monitor`, đặt baud rate `115200`, theo dõi log.

## Kiểm tra đã chạy đúng chưa

- Serial Monitor in ra "Đang kết nối WiFi ... OK".
- Sau vài giây, mở web dashboard AutoSoap — badge **"⚠️ CHẾ ĐỘ MÔ PHỎNG"**
  ở thẻ nhiệt độ sẽ tự biến mất khi ESP gửi được dữ liệu thật.
- Bấm SOS trên web → Serial Monitor phải in "Nhận lệnh: SOS" trong vài giây.

## Lưu ý an toàn quan trọng

Firmware này **không thay thế** mạch dừng khẩn cấp vật lý. E-stop, chống quá
nhiệt và khoá liên động phải là mạch cứng độc lập, cắt nguồn trực tiếp không
đi qua ESP32/WiFi/web. Xem chi tiết trong tài liệu SOP triển khai hệ thống.
