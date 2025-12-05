#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// WiFi credentials
const char* ssid = "Z";
const char* password = "12345678";

// Motor Pins
#define ENA 14
#define IN1 27
#define IN2 26
#define ENB 32
#define IN3 25
#define IN4 33

int speedVal = 200; // 0–255 for normal movements
unsigned long actionEndTime = 0;
bool timedActionRunning = false;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- Motor Functions ----------------
void stopMotor() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  ledcWrite(0, 0); ledcWrite(1, 0);
}

void leftTurn(int s) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(0, s); ledcWrite(1, s);
}

void rightTurn(int s) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(0, s); ledcWrite(1, s);
}

void forward(int s) {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  ledcWrite(0, s); ledcWrite(1, s);
}

void backward(int s) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  ledcWrite(0, s); ledcWrite(1, s);
}

// ---------------- WebSocket ----------------
void handleWsMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
    String msg = "";
    for(size_t i=0;i<len;i++) msg += (char)data[i];
    msg.trim();

    if(msg == "f_press") forward(speedVal);
    else if(msg == "f_release") stopMotor();
    else if(msg == "b_press") backward(speedVal);
    else if(msg == "b_release") stopMotor();
    else if(msg == "l_press") leftTurn(speedVal);
    else if(msg == "l_release") stopMotor();
    else if(msg == "r_press") rightTurn(speedVal);
    else if(msg == "r_release") stopMotor();
    else if(msg.startsWith("speed:")) speedVal = constrain(msg.substring(6).toInt(), 0, 255);
    else if(msg.startsWith("timed:")) {
      int first = msg.indexOf(":");
      int second = msg.indexOf(":", first + 1);
      int third = msg.indexOf(":", second + 1);
      String dir = msg.substring(first+1, second);
      int rotSpeed = msg.substring(second+1, third).toInt();
      float seconds = msg.substring(third+1).toFloat();
      unsigned long duration = (unsigned long)(seconds * 1000.0f);

      if(dir == "l") leftTurn(rotSpeed);
      else if(dir == "r") rightTurn(rotSpeed);

      actionEndTime = millis() + duration;
      timedActionRunning = true;
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT) Serial.printf("Client %u connected\n", client->id());
  else if(type == WS_EVT_DISCONNECT) Serial.printf("Client %u disconnected\n", client->id());
  else if(type == WS_EVT_DATA) handleWsMessage(arg, data, len);
}

// ---------------- HTML Page ----------------
String htmlPage(){
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>🚗 ESP32 Car Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {font-family: Arial,sans-serif; margin:0; padding:0; background:#f0f2f5; text-align:center;}
h2 {margin:20px 0; color:#333;}
.container {display:flex; flex-direction:column; align-items:center; padding:10px;}
.section {background:#fff; border-radius:10px; padding:15px; margin:10px; width:90%; max-width:400px; box-shadow:0 2px 5px rgba(0,0,0,0.2);}
button {width:100%; padding:15px; font-size:18px; margin:5px 0; border:none; border-radius:8px; cursor:pointer; transition:0.2s;}
button:active {transform:scale(0.95); background:#ccc;}
input[type=range], input[type=number] {width:80%; padding:10px; margin:5px 0; border-radius:5px; border:1px solid #ccc;}
label {font-weight:bold; display:block; margin-top:10px;}
#direction-buttons {display:flex; flex-wrap:wrap; justify-content:space-between;}
#direction-buttons button {flex:1 1 45%; margin:5px;}
@media(max-width:500px){button{font-size:16px; padding:12px;} input[type=range],input[type=number]{width:90%;}}
</style>
</head>
<body>
<div class="container">
<h2>🚗 ESP32 Car Control</h2>

<div class="section">
<label>Speed:</label>
<input type="range" min="0" max="255" value="200" id="spd">
<input type="number" id="spdNum" min="0" max="255" value="200">
</div>

<div class="section" id="direction-buttons">
<button id="fBtn" style="background:#4CAF50;color:white;">Forward</button>
<button id="bBtn" style="background:#4CAF50;color:white;">Backward</button>
<button id="lBtn" style="background:#2196F3;color:white;">Left</button>
<button id="rBtn" style="background:#2196F3;color:white;">Right</button>
<button onclick="sendCmd('s')" style="background:#f44336;color:white;">Stop</button>
</div>

<div class="section">
<label>Timed Rotation (seconds):</label>
<input type="number" id="rotSpeed" placeholder="Speed (0-255)" value="100">
<input type="number" id="sec" step="0.1" placeholder="Seconds">
<button onclick="timed('l')">Left</button>
<button onclick="timed('r')">Right</button>
</div>
</div>

<script>
let ws = new WebSocket('ws://' + location.hostname + '/ws');

// Speed slider + number input
let spd=document.getElementById('spd');
let spdNum=document.getElementById('spdNum');
function updateSpeed(val){ ws.send('speed:' + val); spd.value=val; spdNum.value=val; }
spd.addEventListener('input', ()=>{updateSpeed(spd.value);});
spdNum.addEventListener('input', ()=>{updateSpeed(spdNum.value);});

// Momentary button press/release
function addHoldBtn(btn, pressCmd, releaseCmd){
  btn.addEventListener('mousedown', ()=>{ ws.send(pressCmd); });
  btn.addEventListener('mouseup', ()=>{ ws.send(releaseCmd); });
  btn.addEventListener('touchstart', ()=>{ ws.send(pressCmd); });
  btn.addEventListener('touchend', ()=>{ ws.send(releaseCmd); });
}

addHoldBtn(document.getElementById('fBtn'),'f_press','f_release');
addHoldBtn(document.getElementById('bBtn'),'b_press','b_release');
addHoldBtn(document.getElementById('lBtn'),'l_press','l_release');
addHoldBtn(document.getElementById('rBtn'),'r_press','r_release');

// Timed rotation
function timed(dir){
  let t=document.getElementById('sec').value;
  let s=document.getElementById('rotSpeed').value;
  if(t==''||s==''){alert('Enter seconds and speed'); return;}
  ws.send('timed:' + dir + ':' + s + ':' + t);
}

// Generic stop
function sendCmd(c){ ws.send(c); }
</script>
</body>
</html>
)rawliteral";
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  stopMotor();

  ledcSetup(0,1000,8); ledcAttachPin(ENA,0);
  ledcSetup(1,1000,8); ledcAttachPin(ENB,1);

  WiFi.begin(ssid,password);
  Serial.print("Connecting WiFi");
  while(WiFi.status()!=WL_CONNECTED){delay(500); Serial.print(".");}
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200,"text/html",htmlPage()); });
  server.begin();
}

// ---------------- Loop ----------------
void loop(){
  ws.cleanupClients();
  if(timedActionRunning && millis()>actionEndTime){ stopMotor(); timedActionRunning=false;}
}
