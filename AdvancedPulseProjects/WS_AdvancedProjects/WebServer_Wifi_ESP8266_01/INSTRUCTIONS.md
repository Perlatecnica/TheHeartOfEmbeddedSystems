# 🛠 How to Run the ESP8266 LED Controller CLI

This document provides instructions for running the Python CLI application that communicates with the ESP8266-based web server on the STM32 board.

---

## ⚙️ Prerequisites

- Python 3.x installed (recommendation: Python 3.7 or later)
- The ESP8266 must be flashed and running the corresponding web server code.
- Both the **PC and the ESP8266** must be connected to the same WiFi network: **SCEL-net**
- The ESP8266 server must be active and listening on port **80**.

---

## 📁 Project Files

- `esp8266_led_controller.py` → Python CLI script to control the LED
- `README.md` → Project description
- `requirements.txt` → (optional) No external dependencies required

---

## 🚀 How to Run

1. **Open a terminal** in the folder containing the script.

2. **Run the script** with the IP address of the ESP8266:
```bash
python esp8266_led_controller.py <IP_ADDRESS>
```

> Example:
```bash
python esp8266_led_controller.py 192.168.1.123
```

3. You will see a command prompt:
```
ESP8266 LED Controller
Server: http://192.168.1.123

Available commands: ledon, ledoff, status, quit
----------------------------------------
```

4. **Enter commands** to control the LED:
- `ledon` → Turn on the LED
- `ledoff` → Turn off the LED
- `status` → Show the LED state and IP
- `quit` → Exit the program

---

## 🧪 Troubleshooting

- Make sure the IP is correct and reachable
- Ensure the ESP8266 is powered and connected
- If you see a `Connection error`, verify firewall and WiFi settings

---

## 📞 Support

For technical issues, contact your instructor or refer to the UART debug messages printed by the ESP8266 system.


---

## ⚠️ Known Issues

- The script may occasionally show a **timeout error** when trying to reach the ESP8266 server.
- This can happen due to slow WiFi response, server initialization delay, or packet loss.
- **This behavior is acceptable**, as the project is designed purely for **educational purposes** and does not implement full retry logic.

If you encounter a timeout, simply try the command again.
