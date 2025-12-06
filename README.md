# ESP32 Car Control – Web & API Interface

**Author:** Zilani Khan
**Board:** ESP32
**Control:** Web interface (HTTP GET) + RESTful API (HTTP POST with JSON)
**Motors:** L298N / similar H-bridge control

---

## Overview

This project allows you to control a small car powered by an ESP32 using:

1. **Web UI** – Control forward, backward, left, right, stop, and timed rotations using buttons and sliders.
2. **REST API (POST + JSON)** – Integrate control with other apps, scripts, or automation platforms.

The project supports:

* Individual motor commands (`forward`, `backward`, `left`, `right`, `stop`)
* Speed adjustment (0–255)
* Timed rotations (direction + speed + duration in seconds)

---

## Features

* **Dual control**: Web interface for interactive use, JSON API for programmatic control.
* **Non-blocking motor actions**: Uses `millis()` for timed rotations instead of `delay()`.
* **PWM motor speed control**: Smooth motor control using ESP32 `ledcWrite`.
* **Responsive web interface**: Works on desktop and mobile browsers.

---

## Hardware Requirements

* ESP32 Development Board
* Motor Driver (e.g., L298N)
* Two DC motors
* Power supply for motors
* Jumper wires and breadboard

**ESP32 Pin Connections:**

| Motor | IN1 | IN2 | ENA/ENB |
| ----- | --- | --- | ------- |
| Left  | 27  | 26  | 14      |
| Right | 25  | 33  | 32      |

---

## Software Requirements

* Arduino IDE (latest version recommended)
* **Libraries**:

  * `WiFi.h`
  * `WebServer.h`
  * `ArduinoJson.h`

---

## Installation

1. Clone this repository:

   ```bash
   git clone https://github.com/yourusername/esp32-car-control.git
   cd esp32-car-control
   ```

2. Open the `ESP32_Car_Control.ino` file in Arduino IDE.

3. Install required libraries via Arduino Library Manager:

   * **ArduinoJson**
   * **WiFi** (built-in)
   * **WebServer** (built-in)

4. Update WiFi credentials in the code:

   ```cpp
   const char* ssid = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```

5. Select the **ESP32 board** and correct **COM port** in Arduino IDE.

6. Upload the sketch to your ESP32.

---

## Usage

### Web Interface

Open a browser and go to your ESP32 IP, for example:

```
http://192.168.68.100/
```

Use the buttons and sliders to control movement, speed, and timed rotations.

---

## REST API (POST + JSON)

Replace `192.168.68.100` with your ESP32 IP.

### API Endpoint

```
POST http://192.168.68.100/api
Content-Type: application/json
Body: JSON
```

---

### ✅ REST API Examples (cURL Commands)

**1. Forward**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"forward"}'
```

**2. Backward**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"backward"}'
```

**3. Left Turn**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"left"}'
```

**4. Right Turn**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"right"}'
```

**5. Stop**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"stop"}'
```

**6. Change Speed**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"speed":150}'
```

**7. Timed Rotation (Left 2.5s at speed 100)**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"timed":{"dir":"l","speed":100,"seconds":2.5}}'
```

**8. Timed Rotation (Right 3s at speed 120)**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"timed":{"dir":"r","speed":120,"seconds":3}}'
```

**9. Combined Command + Speed**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"command":"forward","speed":200}'
```

**10. Combined Timed + Speed**

```bash
curl -X POST http://192.168.68.100/api \
-H "Content-Type: application/json" \
-d '{"timed":{"dir":"l","speed":150,"seconds":4},"speed":150}'
```

---

## JSON API Structure

**1. Command Only**

```json
{
  "command": "forward"
}
```

**2. Speed Update**

```json
{
  "speed": 200
}
```

**3. Timed Rotation**

```json
{
  "timed": {
    "dir": "r",
    "speed": 150,
    "seconds": 3.0
  }
}
```

You can combine `command`, `speed`, and `timed` in one request.

---

## Notes

* Make sure the motor power supply matches your motors’ voltage rating.
* Use non-blocking control (`millis()`) for timed actions to prevent freezing the server.
* Ensure your ESP32 and client device are on the same WiFi network.

---

## License

This project is **MIT licensed** – feel free to use and modify it for personal projects.


