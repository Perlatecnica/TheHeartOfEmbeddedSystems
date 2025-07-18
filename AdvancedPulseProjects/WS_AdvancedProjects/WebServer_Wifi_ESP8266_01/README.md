# WiFi Scanner and LED Controller with ESP8266 and STM32

## 📋 Project Overview

This embedded application is developed for STM32 microcontrollers and utilizes an ESP8266 WiFi module to perform the following tasks:

- **Scan nearby WiFi networks**
- **Automatically connect to a specific network (`SCEL-net`)**
- **Launch an HTTP web server**
- **Allow remote control of an onboard LED via HTTP commands**

It is intended as an educational tool or a starting point for IoT projects that require wireless connectivity and simple remote interaction with a microcontroller.

---

## 🎯 Goals

- Demonstrate serial communication with an ESP8266 module via AT commands.
- Implement robust WiFi scanning and connection handling.
- Provide a simple HTTP API to control a GPIO pin (LED).
- Enable monitoring and debugging via UART2 (serial console).
- Support both GET and POST HTTP requests.

---

## ⚙️ Hardware Requirements

- STM32-based development board (e.g., Nucleo-F401RE)
- ESP8266 WiFi module (e.g., ESP-01)
- 3.3V power supply for ESP8266 (⚠️ **Do not power with 5V**)
- Level shifter or voltage divider for RX line (if needed)
- Onboard LED connected to `PA5`
- User button on `PC13` for manual WiFi scan trigger

---

## 🔌 UART Configuration

- **UART1**: Communication with ESP8266  
- **UART2**: Debug messages output to serial terminal

---

## 🌐 WiFi Operation

1. The ESP8266 is initialized and communication is verified.
2. A detailed WiFi scan is performed using `AT+CWLAP`.
3. If the target network `SCEL-net` is found:
   - The ESP8266 attempts to connect using the password `Perlatecnica`.
   - IP address is retrieved via `AT+CIFSR`.
4. If connected successfully, the module starts a web server on port **80**.

---

## 🌐 Web API (HTTP Endpoints)

The web server accepts the following HTTP requests:

| Method | Endpoint     | Description                |
|--------|--------------|----------------------------|
| GET    | `/ledon`     | Turns the LED ON           |
| GET    | `/ledoff`    | Turns the LED OFF          |
| GET    | `/status`    | Returns LED status and IP  |
| GET    | `/`          | Lists available endpoints  |
| POST   | body: `ledon` / `ledoff` | Alternative LED control |

**Responses** are in JSON format.

Example response to `GET /status`:
```json
{
  "status": "success",
  "led": "on",
  "ip": "192.168.1.5"
}
```

---

## 🔁 Runtime Behavior

- The blue button on `PC13` can be pressed to manually rescan networks and reconnect.
- LED on `PA5` is ON when the web server is running.
- If disconnected, the LED blinks to indicate server inactivity.
- Periodic status updates are printed every 30 seconds via UART2.

---

## 🧪 Debug & Troubleshooting

All important steps and errors are printed to UART2 (baud rate: **115200**). Monitor this output to:

- Verify ESP8266 responses
- Check WiFi connection status
- See received HTTP requests and responses

Common issues:
- Incorrect SSID or password
- ESP8266 power instability (use stable 3.3V)
- Improper TX/RX wiring
- Firmware incompatibilities

---

## 📂 File Structure

| File        | Description                            |
|-------------|----------------------------------------|
| `main.c`    | Main application logic and HTTP server |
| `esp8266_driver.h` | Driver declarations (assumed external) |
| `main.h`    | MCU configuration and peripheral setup |

---

## ✅ Success Criteria

- The ESP8266 scans for available networks and connects automatically to the desired one.
- The user can control the onboard LED through a browser or any HTTP client.
- Debug messages confirm the correct execution of each step.

---

## 📚 License

This project is distributed under the terms of the MIT License.  
(C) 2024 STMicroelectronics and contributors.
