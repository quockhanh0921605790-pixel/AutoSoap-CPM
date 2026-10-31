#include <WiFi.h>
using fs::FS;
#include <WebServer.h>
#include "esp_timer.h"
#include <Preferences.h>

#define AP_SSID "AutoSoap CPM host"
#define AP_PASS "88888888"

WebServer webServer(80);

// ---- Kết nối Wi-Fi nhà (STA) ----
bool wifiConnecting = false;
bool wifiFailed     = false;
unsigned long wifiConnectStart = 0;
const unsigned long WIFI_CONNECT_TIMEOUT = 20000;   // 20 giây

// ---- Lệnh từ web (được xử lý trong webHostLoop) ----
bool webCmdStop  = false;
bool webCmdStart = false;

// ---- Calculator trên web ----
bool  webCalculated = false;
float webSoap = 0.0;
// ---- Lưu lượng bơm (đặt từ web, lưu vào NVS) ----
Preferences flowPrefs;
const float FLOW_MIN = 0.1;     // g/s
const float FLOW_MAX = 200.0;   // g/s

// =====================================================
// TRANG WEB (nằm trong flash)
// =====================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>AutoSoap CPM</title>
<style>
*{box-sizing:border-box}
body{margin:0;font:16px system-ui,sans-serif;background:#eef0f4;color:#111}
.wrap{max-width:560px;margin:0 auto;padding:12px}
h1{font-size:20px;margin:6px 0 12px;display:flex;justify-content:space-between;align-items:center}
h1 small{font-size:13px;font-weight:400;color:#666}
.card{background:#fff;border-radius:12px;padding:14px;margin-bottom:12px;box-shadow:0 1px 3px #0003}
.card h2{font-size:15px;margin:0 0 10px;color:#555}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-bottom:12px}
.stat{background:#fff;border-radius:12px;padding:10px;text-align:center;box-shadow:0 1px 3px #0003}
.stat span{font-size:12px;color:#666}
.stat b{display:block;font-size:18px;margin-top:4px}
input{width:100%;padding:12px;font-size:18px;border:1px solid #bbb;border-radius:8px;margin-bottom:8px}
button{width:100%;padding:14px;font-size:17px;font-weight:600;border:0;border-radius:10px;cursor:pointer}
button:disabled{opacity:.4;cursor:not-allowed}
.stop{background:#e53935;color:#fff;font-size:20px;padding:16px}
.calc{background:#a7f3d0}
.go{background:#26c6da;margin-top:8px}
.chips{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin:10px 0}
.chip{padding:10px;border-radius:8px;text-align:center;font-size:14px;color:#fff;background:#e53935}
.chip.on{background:#2e7d32}
.bar{height:10px;background:#ddd;border-radius:6px;overflow:hidden;margin:8px 0}
.bar i{display:block;height:100%;width:0;background:#fbc02d}
.muted{color:#666;font-size:14px}
#stName{font-weight:700}
#cRes{white-space:pre-line;margin:6px 0}
.hide{display:none}
.row{display:flex;gap:8px;margin-bottom:10px}
.row input{margin-bottom:0}
.row button{width:90px;flex:none}
.frow{display:flex;justify-content:space-between;margin:4px 0}
</style></head>
<body><div class="wrap">

<div id="vWifi">
  <h1>AutoSoap CPM</h1>
  <div class="card">
    <h2>Kết nối Wi-Fi (internet)</h2>
    <input id="ssid" placeholder="Tên Wi-Fi (SSID)" autocapitalize="off" autocomplete="off">
    <input id="pass" type="password" placeholder="Mật khẩu Wi-Fi">
    <button class="calc" id="bWifi" onclick="wifiGo()">Kết nối</button>
    <p id="wMsg" class="muted"></p>
  </div>
</div>

<div id="vDash" class="hide">
  <h1>AutoSoap CPM <small id="wBadge"></small></h1>

  <div class="stats">
    <div class="stat"><span>Số mẻ</span><b id="sBatch">0</b></div>
    <div class="stat"><span>Hoạt động</span><b id="sUp">--</b></div>
    <div class="stat"><span>Ping</span><b id="sPing">--</b></div>
  </div>

  <button class="stop" style="margin-bottom:12px" onclick="stopAll()">STOP</button>

  <div class="card">
    <h2>Calculator (300 – 500 g)</h2>
    <input id="soap" inputmode="decimal" placeholder="Khối lượng xà phòng (g)" oninput="onSoap(this)">
    <button class="calc" id="bCalc" onclick="calc()">Calculate</button>
    <div id="cRes" class="muted"></div>
    <button class="go" id="bOk" onclick="startRun()" disabled>OK</button>
  </div>

  <div class="card">
    <h2>Quá trình phản ứng</h2>
    <div class="card">
    <h2>Lưu lượng bơm (g/s)</h2>
    <div class="frow"><span>Pump Oil hiện tại</span><b id="fOil">--</b></div>
    <div class="row">
    <input id="inOil" inputmode="decimal" placeholder="Lưu lượng mới" oninput="onFlow(this)">
    <button class="calc" id="bFoil" onclick="setFlow('oil')">Set</button>
    </div>
    <div class="frow"><span>Pump NaOH hiện tại</span><b id="fNaoh">--</b></div>
    <div class="row">
    <input id="inNaoh" inputmode="decimal" placeholder="Lưu lượng mới" oninput="onFlow(this)">
    <button class="calc" id="bFnaoh" onclick="setFlow('naoh')">Set</button>
    </div>
    <div id="fMsg" class="muted"></div>
    </div>
    <div id="stName">--</div>
    <div class="bar" id="barBox"><i id="stBar"></i></div>
    <div id="stTime" class="muted"></div>
    <div class="chips">
      <div class="chip" id="cOil">Pump Oil</div>
      <div class="chip" id="cNaoh">Pump NaOH</div>
      <div class="chip" id="cMix">Mixer</div>
      <div class="chip" id="cPtc">PTC</div>
    </div>
    <div id="stTemp"></div>
    <div id="stTarget" class="muted" style="margin-bottom:12px"></div>
    <button class="stop" onclick="stopAll()">STOP</button>
  </div>
</div>

</div>
<script>
const STATES=["","STATE 1/7 — Phát nhạc, chờ 5 giây","STATE 2/7 — Bơm dầu thải","STATE 3/7 — Tắt bơm dầu, chờ 2 giây","STATE 4/7 — Khởi động máy khuấy, chờ 1 giây","STATE 5/7 — Bơm dung dịch NaOH 30%","STATE 6/7 — Sấy PTC + khuấy","STATE 7/7 — Hoàn thành, về Menu"];
const MODES={menu:"Menu",cal:"màn hình CAL",settings:"Settings",timer:"Timer Calibration",intro:"màn hình khởi động"};
const $=id=>document.getElementById(id);
let S=null,gate=false,calcOk=false;

// fetch có timeout 3 giây (tránh treo khi mất sóng)
function api(url,opt){
  const c=new AbortController(),t=setTimeout(()=>c.abort(),3000);
  return fetch(url,Object.assign({signal:c.signal,cache:"no-store"},opt)).finally(()=>clearTimeout(t));
}
const post=(url,data)=>api(url,{method:"POST",body:new URLSearchParams(data||{})}).then(r=>r.json());
const pad=n=>String(n).padStart(2,"0");
const sec=ms=>(ms/1000).toFixed(1);

function fmtUp(s){
  const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);
  return (d?d+"d ":"")+pad(h)+":"+pad(m)+":"+pad(s%60);
}
function chip(id,on,label){
  const e=$(id);
  e.className="chip"+(on?" on":"");
  e.textContent=label+": "+(on?"ON":"OFF");
}

function render(){
  const w=S.wifi;
  // Đã nối Wi-Fi thì mở giao diện điều khiển, và GIỮ NGUYÊN
  // dù Wi-Fi chập chờn (để không mất nút STOP giữa lúc máy chạy)
  if(w.connected) gate=true;
  $("vWifi").classList.toggle("hide",gate);
  $("vDash").classList.toggle("hide",!gate);
  if(!gate){
    $("bWifi").disabled=w.connecting;
    $("wMsg").textContent=w.connecting?"Đang kết nối...":(w.failed?"Kết nối thất bại. Kiểm tra lại tên và mật khẩu Wi-Fi.":"");
    return;
  }
  $("wBadge").textContent=w.connected?"Wi-Fi: "+w.ssid:"Wi-Fi: mất kết nối";
  $("sBatch").textContent=S.batches;
  $("sUp").textContent=fmtUp(S.up);

  const run=S.mode==="processing",idle=S.mode==="menu";
  if(!idle) calcOk=false;
  $("bCalc").disabled=!idle;
  $("bOk").disabled=!(idle&&calcOk);

  $("stName").textContent=run?STATES[S.state]:"Không chạy — máy đang ở "+(MODES[S.mode]||S.mode);
  const showBar=run&&S.duration>0;
  $("barBox").style.visibility=showBar?"visible":"hidden";
  $("stBar").style.width=showBar?Math.min(100,S.elapsed*100/S.duration)+"%":"0";
  $("stTime").textContent=run?(S.duration>0?sec(S.elapsed)+" / "+sec(S.duration)+" giây":sec(S.elapsed)+" giây"):"";

  chip("cOil",S.oil,"Pump Oil");
  chip("cNaoh",S.naoh,"Pump NaOH");
  chip("cMix",S.mixer,"Mixer");
  chip("cPtc",S.ptc,"PTC");

  let t="Nhiệt độ: ";
  if(!run) t+="--";
  else t+=S.temp===null?"lỗi cảm biến":S.temp.toFixed(1)+" °C";
  if(S.ptcCool>0) t+=" — PTC nghỉ "+Math.ceil(S.ptcCool/1000)+" giây (quá 60 °C)";
  $("stTemp").textContent=t;
  $("stTarget").textContent=run?"Mục tiêu: Waste Oil "+S.oilT.toFixed(1)+" g · NaOH 30% "+S.naohT.toFixed(1)+" g":"";
  $("fOil").textContent=S.oilF.toFixed(2)+" g/s";
  $("fNaoh").textContent=S.naohF.toFixed(2)+" g/s";
  $("bFoil").disabled=run;
  $("bFnaoh").disabled=run;
}

async function getState(){
  try{
    S=await (await api("/api/state")).json();
    render();
  }catch(e){
    $("wBadge").textContent="⚠ mất kết nối với máy";
    $("sPing").textContent="--";
  }
}
// Ping = thời gian khứ hồi trình duyệt <-> ESP32
async function doPing(){
  const t=performance.now();
  try{
    await api("/api/ping");
    $("sPing").textContent=Math.round(performance.now()-t)+" ms";
  }catch(e){
    $("sPing").textContent="--";
  }
}
// Gửi lần lượt (không chồng request) vì ESP32 xử lý từng kết nối một
async function tick(){
  await getState();
  await doPing();
  setTimeout(tick,700);
}

// Ô nhập giống bàn phím TFT: tối đa 3 số + 1 số lẻ, không quá 500
function onSoap(el){
  let v=el.value.replace(",",".");
  if(v[0]===".") v="0"+v;
  if(!/^\d{0,3}(\.\d?)?$/.test(v)||parseFloat(v)>500) v=el.dataset.v||"";
  el.value=v;
  el.dataset.v=v;
  calcOk=false;
  $("bOk").disabled=true;
  $("cRes").textContent="";
}
async function calc(){
  try{
    const r=await post("/api/calc",{soap:$("soap").value});
    if(r.ok){
      calcOk=true;
      $("bOk").disabled=false;
      $("cRes").textContent="Xà phòng: "+r.soap.toFixed(1)+" g\nWaste Oil: "+r.oil.toFixed(1)+" g\nNaOH 30%: "+r.naoh.toFixed(1)+" g";
    }else{
      $("cRes").textContent=r.err==="range"?"Lỗi: chỉ nhận 300 – 500 g":(r.err==="busy"?"Máy đang bận, hãy đợi về Menu":"Chưa nhập khối lượng");
    }
  }catch(e){
    $("cRes").textContent="Không gửi được lệnh tới máy";
  }
}
async function startRun(){
  $("bOk").disabled=true;
  try{
    const r=await post("/api/start");
    if(r.ok) calcOk=false;
    else $("cRes").textContent="Không bắt đầu được (máy bận hoặc chưa Calculate)";
  }catch(e){
    $("cRes").textContent="Không gửi được lệnh tới máy";
  }
}
// STOP thử lại tối đa 3 lần; thất bại thì báo để bấm STOP trên máy
async function stopAll(){
  for(let i=0;i<3;i++){
    try{
      const r=await post("/api/stop");
      if(r.ok) return;
    }catch(e){}
  }
  alert("Không gửi được lệnh STOP! Hãy bấm STOP trên màn hình máy.");
}
async function wifiGo(){
  const ssid=$("ssid").value.trim(),pass=$("pass").value;
  if(!ssid){ $("wMsg").textContent="Hãy nhập tên Wi-Fi"; return; }
  $("wMsg").textContent="Đang kết nối...";
  try{ await post("/api/wifi",{ssid:ssid,pass:pass}); }catch(e){}
}
// Ô nhập lưu lượng: tối đa 3 số + 2 số lẻ, không quá 200
function onFlow(el){
  let v=el.value.replace(",",".");
  if(v[0]===".") v="0"+v;
  if(!/^\d{0,3}(\.\d{0,2})?$/.test(v)||parseFloat(v)>200) v=el.dataset.v||"";
  el.value=v;
  el.dataset.v=v;
}
async function setFlow(p){
  const el=$(p==="oil"?"inOil":"inNaoh");
  const name=p==="oil"?"Pump Oil":"Pump NaOH";
  try{
    const r=await post("/api/flow",{pump:p,value:el.value});
    if(r.ok){
      el.value="";
      el.dataset.v="";
      $("fMsg").textContent="Đã cập nhật lưu lượng "+name;
    }else if(r.err==="range"){
      $("fMsg").textContent="Lỗi: "+name+" chỉ nhận "+r.min+" – "+r.max+" g/s";
    }else if(r.err==="busy"){
      $("fMsg").textContent="Máy đang chạy, hãy đổi sau khi xong mẻ";
    }else{
      $("fMsg").textContent="Lệnh không hợp lệ";
    }
  }catch(e){
    $("fMsg").textContent="Không gửi được lệnh tới máy";
  }
}
tick();
</script></body></html>
)rawliteral";


// =====================================================
// HÀM PHỤ
// =====================================================

void sendJson(const String &body) {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", body);
}

// Escape " và \ để tên Wi-Fi không làm hỏng JSON
String jsonEscape(const String &s) {
  String o;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c < 0x20) { o += ' '; }
    else { o += c; }
  }
  return o;
}

const char* webModeName() {
  switch (currentMode) {
    case MODE_INTRO:             return "intro";
    case MODE_MENU:              return "menu";
    case MODE_CAL:               return "cal";
    case MODE_PROCESSING:        return "processing";
    case MODE_SETTINGS:          return "settings";
    case MODE_TIMER_CALIBRATION: return "timer";
  }
  return "?";
}

// Thời gian dự kiến của bước hiện tại (ms) để vẽ thanh tiến trình.
// Các bước ngắn (STATE 1, 3, 4, 7) trả 0 -> web chỉ hiện số giây đã chạy.
unsigned long webStateDuration() {
  switch (processingState) {
    case PROC_OIL_ON:  return oilPumpTime;
    case PROC_NAOH_ON: return naohPumpTime;
    case PROC_MIXING:  return MIXING_TIME;
    default:           return 0;
  }
}


// =====================================================
// HANDLERS
// =====================================================

void handleRoot() {
  webServer.send_P(200, "text/html", INDEX_HTML);
}

void handlePing() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/plain", "ok");
}

// Gói toàn bộ trạng thái máy thành 1 JSON
void handleState() {

  unsigned long now = millis();
  bool wifiOk     = (WiFi.status() == WL_CONNECTED);
  bool processing = (currentMode == MODE_PROCESSING);
  bool tempOk     = processing && (processingTemperature > -100.0);

  // Thời gian PTC còn phải nghỉ (cooldown 10 giây)
  unsigned long ptcLeft = 0;
  if (processing && ptcCooldown) {
    unsigned long el = now - ptcCooldownStart;
    if (el < PTC_COOLDOWN_TIME) ptcLeft = PTC_COOLDOWN_TIME - el;
  }

  String j;
  j.reserve(480);

  j += "{\"wifi\":{\"connected\":";   j += wifiOk ? "true" : "false";
  j += ",\"connecting\":";            j += wifiConnecting ? "true" : "false";
  j += ",\"failed\":";                j += wifiFailed ? "true" : "false";
  j += ",\"ssid\":\"";                j += wifiOk ? jsonEscape(WiFi.SSID()) : String("");
  j += "\",\"ip\":\"";                j += wifiOk ? WiFi.localIP().toString() : String("");
  j += "\"}";

  // Uptime tính bằng esp_timer (64-bit) nên không bị tràn sau 49 ngày như millis()
  j += ",\"up\":";        j += (unsigned long)(esp_timer_get_time() / 1000000ULL);
  j += ",\"batches\":";   j += batchCount;
  j += ",\"mode\":\"";    j += webModeName();
  j += "\",\"state\":";   j += processing ? (int)processingState + 1 : 0;
  j += ",\"elapsed\":";   j += processing ? (now - processingTimer) : 0UL;
  j += ",\"duration\":";  j += processing ? webStateDuration() : 0UL;

  j += ",\"oil\":";       j += pumpOilState ? "true" : "false";
  j += ",\"naoh\":";      j += pumpNaOHState ? "true" : "false";
  j += ",\"mixer\":";     j += mixerState ? "true" : "false";
  j += ",\"ptc\":";       j += ptcState ? "true" : "false";
  j += ",\"ptcCool\":";   j += ptcLeft;

  j += ",\"temp\":";      j += tempOk ? String(processingTemperature, 1) : String("null");
  j += ",\"oilT\":";      j += String(oilValue, 1);
  j += ",\"naohT\":";     j += String(naohSolutionValue, 1);
  j += ",\"oilF\":";      j += String(OIL_FLOW_RATE, 2);
  j += ",\"naohF\":";     j += String(NAOH_FLOW_RATE, 2);
  j += "}";

  sendJson(j);
}

// Nhận tên + mật khẩu Wi-Fi, bắt đầu kết nối (KHÔNG chờ, không block)
void handleWifi() {

  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");

  if (ssid.length() == 0 || ssid.length() > 32 || pass.length() > 63) {
    sendJson("{\"ok\":false}");
    return;
  }

  Serial.print("WEB: connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.disconnect();                       // hủy lần thử cũ (AP vẫn chạy)
  WiFi.begin(ssid.c_str(), pass.c_str());

  wifiConnecting   = true;
  wifiFailed       = false;
  wifiConnectStart = millis();

  sendJson("{\"ok\":true}");
}

// Calculator đảo: nhập khối lượng xà phòng -> ra dầu + NaOH 30%
void handleCalc() {

  // Chỉ tính khi máy rảnh ở Menu (tránh đè dữ liệu màn hình CAL/Processing)
  if (currentMode != MODE_MENU) {
    sendJson("{\"ok\":false,\"err\":\"busy\"}");
    return;
  }

  String s = webServer.arg("soap");

  if (s.length() == 0) {
    webCalculated = false;
    sendJson("{\"ok\":false,\"err\":\"empty\"}");
    return;
  }

  float v = s.toFloat();

  // Cùng điều kiện màn hình CAL: 300 – 500 g.
  // Viết dạng !(a && b) để giá trị NaN cũng bị loại.
  if (!(v >= 300.0 && v <= 500.0)) {
    webCalculated = false;
    sendJson("{\"ok\":false,\"err\":\"range\"}");
    return;
  }

  // Dùng đúng hàm calculateSoap() của test_core.ino
  inputText = String(v, 1);
  calculateSoap();

  webSoap       = v;
  webCalculated = true;

  String j = "{\"ok\":true,\"soap\":";
  j += String(soapValue, 1);
  j += ",\"oil\":";   j += String(oilValue, 1);
  j += ",\"naoh\":";  j += String(naohSolutionValue, 1);
  j += "}";
  sendJson(j);
}

// "OK": chỉ đặt cờ, việc chạy máy làm trong webHostLoop()
void handleStart() {

  if (currentMode != MODE_MENU || !webCalculated) {
    sendJson("{\"ok\":false,\"err\":\"busy\"}");
    return;
  }

  webCmdStart = true;
  sendJson("{\"ok\":true}");
}

void handleStop() {
  webCmdStop = true;
  sendJson("{\"ok\":true}");
}

// =====================================================
// LƯU LƯỢNG BƠM (g/s)
// =====================================================

void loadFlowRates() {

  flowPrefs.begin("autosoap", true);   // true = chỉ đọc

  float o = flowPrefs.getFloat("oilFlow",  OIL_FLOW_RATE);
  float n = flowPrefs.getFloat("naohFlow", NAOH_FLOW_RATE);

  flowPrefs.end();

  // Chỉ nhận giá trị nằm trong khoảng hợp lệ
  if (o >= FLOW_MIN && o <= FLOW_MAX) OIL_FLOW_RATE  = o;
  if (n >= FLOW_MIN && n <= FLOW_MAX) NAOH_FLOW_RATE = n;

  Serial.print("Flow oil: ");
  Serial.print(OIL_FLOW_RATE, 2);
  Serial.print(" g/s | Flow NaOH: ");
  Serial.print(NAOH_FLOW_RATE, 2);
  Serial.println(" g/s");
}

void saveFlowRates() {

  flowPrefs.begin("autosoap", false);  // false = cho phép ghi

  flowPrefs.putFloat("oilFlow",  OIL_FLOW_RATE);
  flowPrefs.putFloat("naohFlow", NAOH_FLOW_RATE);

  flowPrefs.end();
}

// POST /api/flow  (pump = oil | naoh, value = g/s)
void handleFlow() {

  // Đang chạy mẻ thì không cho đổi (xem giải thích ở trên)
  if (currentMode == MODE_PROCESSING) {
    sendJson("{\"ok\":false,\"err\":\"busy\"}");
    return;
  }

  String pump = webServer.arg("pump");
  float  v    = webServer.arg("value").toFloat();

  // Viết dạng !(a && b) để NaN cũng bị loại; chuỗi rỗng -> 0 -> bị loại
  if (!(v >= FLOW_MIN && v <= FLOW_MAX)) {
    String j = "{\"ok\":false,\"err\":\"range\",\"min\":";
    j += String(FLOW_MIN, 1);
    j += ",\"max\":";
    j += String(FLOW_MAX, 1);
    j += "}";
    sendJson(j);
    return;
  }

  // Làm tròn 2 số lẻ
  v = roundf(v * 100.0f) / 100.0f;

  if (pump == "oil") {
    OIL_FLOW_RATE = v;
  }
  else if (pump == "naoh") {
    NAOH_FLOW_RATE = v;
  }
  else {
    sendJson("{\"ok\":false,\"err\":\"bad\"}");
    return;
  }

  saveFlowRates();

  Serial.print("WEB: flow ");
  Serial.print(pump);
  Serial.print(" = ");
  Serial.print(v, 2);
  Serial.println(" g/s");

  sendJson("{\"ok\":true}");
}

// =====================================================
// BEGIN (gọi 1 lần trong setup)
// =====================================================

void webHostBegin() {

  WiFi.mode(WIFI_AP_STA);
  loadFlowRates();

  // Tắt ngủ Wi-Fi để ping ổn định, phản hồi nhanh
  WiFi.setSleep(false);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS);

  Serial.print("AP ");
  Serial.print(ok ? "OK: " : "FAIL: ");
  Serial.println(AP_SSID);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  webServer.on("/",          HTTP_GET,  handleRoot);
  webServer.on("/api/ping",  HTTP_GET,  handlePing);
  webServer.on("/api/state", HTTP_GET,  handleState);
  webServer.on("/api/wifi",  HTTP_POST, handleWifi);
  webServer.on("/api/calc",  HTTP_POST, handleCalc);
  webServer.on("/api/start", HTTP_POST, handleStart);
  webServer.on("/api/stop",  HTTP_POST, handleStop);
  webServer.on("/api/flow",  HTTP_POST, handleFlow);

  webServer.onNotFound([]() {
    webServer.send(404, "text/plain", "Not found");
  });

  webServer.begin();
  Serial.println("WEB SERVER READY");
}


// =====================================================
// LOOP (gọi mỗi vòng loop())
// =====================================================

void webHostLoop() {

  webServer.handleClient();

  // ---- Theo dõi kết nối Wi-Fi nhà ----
  if (wifiConnecting) {

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnecting = false;
      wifiFailed     = false;
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());
    }
    else if (millis() - wifiConnectStart >= WIFI_CONNECT_TIMEOUT) {
      wifiConnecting = false;
      wifiFailed     = true;
      WiFi.disconnect();
      Serial.println("WiFi connect FAILED");
    }
  }

    // ---- Vẽ lại dòng IP Wi-Fi trên Menu khi kết nối đổi (nối mới / mất mạng / đổi IP) ----
  static IPAddress lastWifiIp(0, 0, 0, 0);
  IPAddress nowWifiIp = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP() : IPAddress(0, 0, 0, 0);

  if (nowWifiIp != lastWifiIp) {
    lastWifiIp = nowWifiIp;
    if (currentMode == MODE_MENU) drawWifiIpLine();
  }

  // Rời Menu thì kết quả Calculate trên web hết hiệu lực
  if (currentMode != MODE_MENU) webCalculated = false;

  // ---- STOP (ưu tiên cao nhất) ----
  if (webCmdStop) {

    webCmdStop  = false;
    webCmdStart = false;     // có STOP thì hủy luôn lệnh START đang chờ

    Serial.println();
    Serial.println("============================");
    Serial.println("WEB: STOP PRESSED");
    Serial.println("============================");

    // Y như nút STOP trên màn hình
    setPumpOil(false);
    setPumpNaOH(false);
    setMixer(false);
    setPTC(false);

    Serial.println("ALL MODULES OFF");

    // Đang ở Menu thì khỏi vẽ lại; Intro tự chuyển sang Menu
    if (currentMode != MODE_MENU && currentMode != MODE_INTRO) {
      Serial.println("WEB: -> MENU");
      enterMenu();
    }
  }

  // ---- START (nút OK trên web) ----
  if (webCmdStart) {

    webCmdStart = false;

    if (currentMode == MODE_MENU && webCalculated) {

      // Tính lại cho chắc: lỡ màn hình CAL đã reset oilValue/naohSolutionValue
      inputText = String(webSoap, 1);
      calculateSoap();

      Serial.println("WEB: OK -> PROCESSING");
      enterProcessing();
    }
  }
}

// =====================================================
// DÒNG IP WI-FI NHÀ (nằm ngay trên dòng IP của ESP32)
// =====================================================

void drawWifiIpLine() {

  // Xóa dòng cũ. Chỉ rộng 132 px (x = 8..140) để không đè lên nút START (bắt đầu từ x = 140)
  tft.fillRect(8, 248, 132, 16, TFT_WHITE);

  // Chưa kết nối thì để trống
  if (WiFi.status() != WL_CONNECTED) return;

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  String ip   = WiFi.localIP().toString();
  String line = "WiFi: " + ip;

  // IP dài (vd 192.168.100.123) làm dòng tràn sang nút START -> bỏ nhãn "WiFi: "
  if (tft.textWidth(line, 2) > 132) line = ip;

  tft.drawString(line, 8, 248, 2);
}

// =====================================================
// HIỂN THỊ IP + PASS Ở GÓC DƯỚI TRÁI MENU
// =====================================================

void drawWebInfo() {

  drawWifiIpLine();

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  tft.drawString("IP: " + WiFi.softAPIP().toString(), 8, 268, 2);
  tft.drawString("Pass: " + String(AP_PASS),          8, 288, 2);
}
