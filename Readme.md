# ESP32 Firebase Display Project

This project uses an ESP32 board and a small LCD screen. It shows messages from Firebase. It updates every 10 seconds.

---

## ✅ What it does

- Connects to WiFi
- Gets time from internet
- Gets messages from Firebase
- Shows messages on LCD (160x128)
- Refreshes message every 10 seconds

---

## 📦 What you need

- ESP32 development board
- 1.8" SPI LCD screen (160x128)
- Jumper wires
- Computer with ESP-IDF and CMake installed

---


## ⚙️ Setup Instructions

### 1. WiFi Settings

Edit `main.c`:

```c
#define WIFI_SSID "Your_WiFi_Name"
#define WIFI_PASS "Your_WiFi_Password"
```

### 2. Firebase Settings

Edit `rest_client.c`:

```c
#define FIREBASE_URL "Your_Firebase_URL_Here"
```

---

## 🚀 How to Start

1. Connect ESP32 to your computer
2. Open terminal in the project folder
3. Run:

```bash
idf.py set-target esp32
idf.py build
idf.py -p PORT flash monitor
```

(Replace `PORT` with your ESP32 port, like COM3 or /dev/ttyUSB0)

---


## ☁️ Sending Messages to Firebase

1. Open your Firebase Realtime Database
2. Add a message at the path `/latest`

Example:

```json
{
  "latest": "Hello World!"
}
```

ESP32 will show this on the LCD.

---

## 🪰 Troubleshooting

- **Screen is blank?**  
  Check the wire connections.

- **Stuck on "Connecting..."?**  
  Check your WiFi SSID and password.

- **Shows "Fetch error"?**  
  Check your Firebase URL.

---

## 🧠 What I learned

- How to connect ESP32 to WiFi
- How to get time from internet
- How to read data from Firebase
- How to display messages on an LCD
- How to structure a small embedded project

---

## 💬 Easy Explanation

"This project uses ESP32 to get a message from the internet. It shows this message on a small screen. It checks for a new message every 10 seconds."
