#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ---------------- WiFi ----------------
const char* ssid = "Z";
const char* password = "12345678";

// ---------------- Motor Pins ----------------
#define ENA 14
#define IN1 27
#define IN2 26
#define ENB 32
#define IN3 25
#define IN4 33

WebServer server(80);

int speedVal = 200; 
unsigned long actionEndTime = 0;
bool timedActionRunning = false;

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

// ---------------- Command Handler (Web GET) ----------------
void handleCommand() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "Missing command");
    return;
  }

  String cmd = server.arg("c");
  cmd.trim();

  // Basic motor commands (press/release)
  if (cmd == "f_press") forward(speedVal);
  else if (cmd == "f_release") stopMotor();
  else if (cmd == "b_press") backward(speedVal);
  else if (cmd == "b_release") stopMotor();
  else if (cmd == "l_press") leftTurn(speedVal);
  else if (cmd == "l_release") stopMotor();
  else if (cmd == "r_press") rightTurn(speedVal);
  else if (cmd == "r_release") stopMotor();
  else if (cmd == "s") stopMotor();

  // Speed update
  else if (cmd.startsWith("speed:")) {
    int v = cmd.substring(6).toInt();
    speedVal = constrain(v, 0, 255);
  }

  // Timed rotation: timed:dir:rotSpeed:seconds
  else if (cmd.startsWith("timed:")) {
    int a = cmd.indexOf(":", 0);
    int b = cmd.indexOf(":", a + 1);
    int c = cmd.indexOf(":", b + 1);
    if (a < 0 || b < 0 || c < 0) {
      server.send(400, "text/plain", "Bad timed format");
      return;
    }
    String dir = cmd.substring(a + 1, b);
    int rotSpeed = cmd.substring(b + 1, c).toInt();
    float sec = cmd.substring(c + 1).toFloat();

    unsigned long dur = (unsigned long)(sec * 1000.0f);

    if (dir == "l") leftTurn(rotSpeed);
    else if (dir == "r") rightTurn(rotSpeed);

    actionEndTime = millis() + dur;
    timedActionRunning = true;
  }

  server.send(200, "text/plain", "OK");
}

// ---------------- REST API POST ----------------
void handleAPI() {
  if (server.method() != HTTP_POST) {
    server.send(405, "application/json", "{\"error\":\"POST method required\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  // Motor command
  if (doc.containsKey("command")) {
    String cmd = doc["command"].as<String>();

    if (cmd == "forward") forward(speedVal);
    else if (cmd == "backward") backward(speedVal);
    else if (cmd == "left") leftTurn(speedVal);
    else if (cmd == "right") rightTurn(speedVal);
    else if (cmd == "stop") stopMotor();
    else {
      server.send(400, "application/json", "{\"error\":\"Unknown command\"}");
      return;
    }
  }

  // Optional speed update
  if (doc.containsKey("speed")) {
    int spd = doc["speed"];
    speedVal = constrain(spd, 0, 255);
  }

  // Optional timed action
  if (doc.containsKey("timed")) {
    JsonObject t = doc["timed"];
    String dir = t["dir"] | "";
    int rotSpeed = t["speed"] | 100;
    float sec = t["seconds"] | 1.0;

    unsigned long dur = (unsigned long)(sec * 1000.0f);

    if (dir == "l") leftTurn(rotSpeed);
    else if (dir == "r") rightTurn(rotSpeed);

    actionEndTime = millis() + dur;
    timedActionRunning = true;
  }

  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

// ---------------- HTML Page ----------------
String htmlPage() {
  return R"rawliteral(
<!DOCTYPE html><html><head>
<title>ESP32 Car Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {font-family:Arial; background:#f0f2f5; text-align:center; margin:0; padding:0;}
.container {max-width:460px; margin:12px auto; padding:10px;}
.section {background:white; padding:14px; margin:10px; border-radius:10px; box-shadow:0 2px 6px rgba(0,0,0,0.12);}
button {padding:12px; width:48%; margin:6px 1%; font-size:16px; border:none; border-radius:8px; cursor:pointer; background:#4CAF50; color:white;}
button.small {width:30%; padding:10px; font-size:14px;}
button.stop {background:#f44336;}
button.turn {background:#2196F3;}
input[type=range], input[type=number] {width:70%; padding:8px; margin:6px 0;}
label{display:block; text-align:left; margin-left:5%; font-weight:bold;}
@media(max-width:480px){
  button{font-size:14px; padding:10px;}
  input[type=range], input[type=number]{width:85%;}
}
</style>
</head><body>
<div class="container">
  <h2>ESP32 Car Control (HTTP)</h2>

  <div class="section">
    <label>Speed (normal movements)</label>
    <input id="spd" type="range" min="0" max="255" value="200">
    <input id="spdNum" type="number" min="0" max="255" value="200">
  </div>

  <div class="section">
    <button onmousedown="send('f_press')" onmouseup="send('f_release')" ontouchstart="send('f_press')" ontouchend="send('f_release')">Forward</button>
    <button onmousedown="send('b_press')" onmouseup="send('b_release')" ontouchstart="send('b_press')" ontouchend="send('b_release')">Backward</button>
    <button class="turn" onmousedown="send('l_press')" onmouseup="send('l_release')" ontouchstart="send('l_press')" ontouchend="send('l_release')">Left</button>
    <button class="turn" onmousedown="send('r_press')" onmouseup="send('r_release')" ontouchstart="send('r_press')" ontouchend="send('r_release')">Right</button>
    <button class="stop" onclick="send('s')">STOP</button>
  </div>

  <div class="section">
    <label>Timed Rotation (dir:speed:seconds)</label>
    <input id="rotSpeed" type="number" min="0" max="255" value="100" placeholder="Rotation speed (0-255)">
    <input id="sec" type="number" step="0.1" placeholder="Seconds">
    <div style="margin-top:8px;">
      <button class="small" onclick="timed('l')">Rotate Left</button>
      <button class="small" onclick="timed('r')">Rotate Right</button>
    </div>
  </div>
</div>

<script>
function send(cmd){
  fetch('/cmd?c=' + encodeURIComponent(cmd)).catch(err => console.log('req err', err));
}

let spd = document.getElementById('spd');
let spdNum = document.getElementById('spdNum');

function updateSpeed(val){
  spd.value = val;
  spdNum.value = val;
  send('speed:' + val);
}

spd.addEventListener('input', ()=> updateSpeed(spd.value));
spdNum.addEventListener('input', ()=> updateSpeed(spdNum.value));

function timed(dir){
  let s = document.getElementById('rotSpeed').value;
  let t = document.getElementById('sec').value;
  if (s === '' || t === '') { alert('Enter rotation speed and seconds'); return; }
  send('timed:' + dir + ':' + s + ':' + t);
}
</script>
</body></html>
)rawliteral";
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);

  // configure pins
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // configure PWM channels for ENA and ENB
  ledcSetup(0, 1000, 8); 
  ledcAttachPin(ENA, 0);
  ledcSetup(1, 1000, 8); 
  ledcAttachPin(ENB, 1);

  stopMotor();

  // connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // HTTP GET web server
  server.on("/", HTTP_GET, []() { server.send(200, "text/html", htmlPage()); });
  server.on("/cmd", HTTP_GET, handleCommand);

  // HTTP POST API server
  server.on("/api", HTTP_POST, handleAPI);

  server.begin();
  Serial.println("HTTP & API server started");
}

// ---------------- Loop ----------------
void loop() {
  server.handleClient();

  if (timedActionRunning && millis() > actionEndTime) {
    stopMotor();
    timedActionRunning = false;
  }
}
