# 🔔 ESP8266 Notification Buzzer Setup Guide

## Overview
This guide will help you set up an ESP8266 with a piezo buzzer that triggers physical notifications when your DiaWeb app sends alerts through Firebase.

---

## 📦 Hardware Requirements

### Required Components
1. **ESP8266 Development Board** (Choose one):
   - NodeMCU ESP8266 (Recommended for beginners)
   - Wemos D1 Mini
   - ESP-12E/F module with adapter

2. **Piezo Buzzer**
   - Active or Passive Buzzer (5V tolerant)
   - Recommended: Passive buzzer for tone control

3. **Jumper Wires**
   - Male-to-Male or Male-to-Female (depending on your board)

4. **USB Cable**
   - Micro-USB for NodeMCU/Wemos D1 Mini

5. **Optional Components**
   - Breadboard (for prototyping)
   - 100Ω Resistor (to reduce buzzer volume if needed)
   - Transistor (2N2222 or similar) for louder sound

---

## 🔌 Wiring Diagram

### Basic Connection (Direct)
```
ESP8266 (NodeMCU)          Piezo Buzzer
├─ D1 (GPIO5) ────────────── Positive (+)
└─ GND ───────────────────── Negative (-)
```

### With Volume Control (Resistor)
```
ESP8266 (NodeMCU)          Components
├─ D1 (GPIO5) ──── 100Ω Resistor ──── Buzzer (+)
└─ GND ────────────────────────────── Buzzer (-)
```

### Enhanced Circuit (Louder - with Transistor)
```
ESP8266 (NodeMCU)          2N2222 Transistor        Buzzer
├─ D1 (GPIO5) ──── 1kΩ ──── Base
│                            Collector ───────────── (+) 5V
│                            Emitter ─────────────── Buzzer (+)
└─ GND ─────────────────────────────────────────── Buzzer (-)
```

### Pin Mapping Reference
| Board Pin | GPIO | Function |
|-----------|------|----------|
| D0 | GPIO16 | Built-in LED (some boards) |
| D1 | GPIO5 | **Buzzer Output** |
| D2 | GPIO4 | Alternative buzzer pin |
| D3 | GPIO0 | Boot mode (avoid using) |
| D4 | GPIO2 | Built-in LED |
| GND | GND | Ground |
| 3V3 | 3.3V | Power output |

---

## 💻 Software Setup

### Step 1: Install Arduino IDE
1. Download Arduino IDE from [arduino.cc](https://www.arduino.cc/en/software)
2. Install the IDE on your computer
3. Open Arduino IDE

### Step 2: Install ESP8266 Board Support
1. Open **File → Preferences**
2. In "Additional Board Manager URLs", add:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Click **OK**
4. Go to **Tools → Board → Boards Manager**
5. Search for "esp8266"
6. Install **"esp8266 by ESP8266 Community"**
7. Wait for installation to complete

### Download and Install Manually

1. **Download the Library**
   - Go to: https://github.com/mobizt/Firebase-ESP-Client
   - Click the green **"Code"** button
   - Select **"Download ZIP"**

2. **Install the ZIP**
   - In Arduino IDE: `Sketch` → `Include Library` → `Add .ZIP Library...`
   - Navigate to your Downloads folder
   - Select `Firebase-ESP-Client-main.zip`
   - Click **"Open"**

3. **Restart Arduino IDE**

---

## Updated Arduino Code (Use This Instead)

The library name has changed. Here's the corrected code:

```cpp
### Step 4: Select Your Board
1. Go to **Tools → Board**
2. Select your ESP8266 board:
   - **NodeMCU 1.0 (ESP-12E Module)** for NodeMCU
   - **LOLIN(WEMOS) D1 R2 & mini** for Wemos D1 Mini

### Step 5: Configure Upload Settings
**Tools Menu Settings:**
```
Board: "NodeMCU 1.0 (ESP-12E Module)"
Upload Speed: "115200"
CPU Frequency: "80 MHz"
Flash Size: "4MB (FS:2MB OTA:~1019KB)"
Port: Select your COM port (e.g., COM3, COM4)
```

---

## 🔧 Firebase Configuration

### Step 1: Get Firebase Database Secret
1. Go to [Firebase Console](https://console.firebase.google.com)
2. Select your DiaWeb project
3. Click the **Gear Icon** → **Project Settings**
4. Go to **Service Accounts** tab
5. Click **Database Secrets**
6. Copy your database secret key

### Step 2: Get Firebase Host
Your Firebase host is: `YOUR_PROJECT_ID.firebaseio.com`
- Find PROJECT_ID in Firebase Console under Project Settings

### Step 3: Get User ID
Option 1 - From Web Console:
1. Open Firebase Console
2. Go to **Realtime Database**
3. Navigate to `/users/`
4. Copy the user ID (the long string under users)

Option 2 - From Browser:
1. Login to your DiaWeb web app
2. Open browser Developer Tools (F12)
3. Go to Console tab
4. Type: `firebase.auth().currentUser.uid`
5. Copy the returned ID

---

## ⚙️ Code Configuration

### Step 1: Open the Arduino Sketch
1. Open `ESP8266_Buzzer.ino` in Arduino IDE

### Step 2: Update WiFi Credentials
```cpp
#define WIFI_SSID "Nutella"      // Your WiFi network name
#define WIFI_PASSWORD "JaveJayne0306!"  // Your WiFi password
```

### Step 3: Update Firebase Credentials
```cpp
#define FIREBASE_HOST "your-project.firebaseio.com"  // Without https://
#define FIREBASE_AUTH "your_database_secret_key"     // From Firebase console
```

### Step 4: Set Your User ID
```cpp
#define USER_ID "abc123xyz456..."  // Your actual user ID
```

### Step 5: Verify Pin Configuration
```cpp
#define BUZZER_PIN D1        // GPIO5 - Change if using different pin
#define LED_PIN LED_BUILTIN  // Built-in LED
```

---

## 📤 Upload to ESP8266

### Step 1: Connect ESP8266
1. Connect ESP8266 to computer via USB cable
2. Wait for driver installation (if first time)
3. Note the COM port number

### Step 2: Select Port
1. In Arduino IDE: **Tools → Port**
2. Select the correct COM port (e.g., COM3)

### Step 3: Upload Code
1. Click the **Upload** button (→) or press **Ctrl+U**
2. Wait for "Connecting..." message
3. Wait for upload to complete
4. You should see: "Hard resetting via RTS pin..."

### Step 4: Monitor Serial Output
1. Open **Tools → Serial Monitor**
2. Set baud rate to **115200**
3. You should see:
   ```
   =================================
   DiaWeb Notification Buzzer
   =================================
   Connecting to WiFi: Your_Network
   ........
   WiFi Connected!
   IP Address: 192.168.1.x
   Setup complete!
   Monitoring for notifications...
   ```

---

## 🧪 Testing

### Test 1: Welcome Beep
- When ESP8266 starts, it should play a rising tone (3 beeps)
- This confirms the buzzer is working

### Test 2: Web App Notification
1. Open your DiaWeb web app
2. Add a blood sugar reading:
   - **Below 70 mg/dL** → Should trigger LOW GLUCOSE alert (fast beeping)
   - **Above 180 mg/dL** → Should trigger HIGH GLUCOSE alert (4 beeps)
   - **Normal range** → No buzzer notification

### Test 3: Medication Reminder
1. Add a medication with time = current time + 1 minute
2. Wait for the time
3. Buzzer should play medication pattern (3 short beeps)

### Test 4: Custom Reminder
1. Add a reminder with time = current time + 1 minute
2. Wait for the time
3. Buzzer should play reminder pattern (2 long beeps)

---

## 🔊 Buzzer Patterns

| Notification Type | Pattern | Description |
|-------------------|---------|-------------|
| **Medication** | 🎵 Beep-Beep-Beep | 3 short beeps (200ms each) |
| **Reminder** | 🎵 Beeeep-Beeeep | 2 long beeps (500ms each) |
| **High Glucose** | 🎵 Beep-Beep-Beep-Beep | 4 medium beeps (150ms) |
| **Low Glucose** | 🎵🚨 BEEP-BEEP-BEEP-BEEP-BEEP-BEEP | 6 fast urgent beeps (100ms) |
| **Welcome** | 🎵 Do-Re-Mi | Rising tones on startup |

---

## 🔍 Troubleshooting

### ESP8266 Won't Connect to WiFi
**Problem**: Stuck on "Connecting to WiFi..."

**Solutions**:
1. Double-check WiFi credentials (case-sensitive)
2. Ensure WiFi is 2.4GHz (ESP8266 doesn't support 5GHz)
3. Move ESP8266 closer to WiFi router
4. Check if WiFi has MAC address filtering enabled

### Upload Failed / Port Not Found
**Problem**: "Couldn't find board on selected port"

**Solutions**:
1. Install CH340/CP2102 USB drivers (search for your board type)
2. Try different USB cable (some are charge-only)
3. Try different USB port on computer
4. Hold FLASH button while uploading (on some boards)

### Firebase Connection Error
**Problem**: "Failed to get notifications: connection refused"

**Solutions**:
1. Verify Firebase host format: `project.firebaseio.com` (no https://)
2. Check Firebase Database Secret key is correct
3. Ensure Firebase Realtime Database is created (not just Firestore)
4. Check Firebase Rules allow read access

### Buzzer Not Working
**Problem**: No sound from buzzer

**Solutions**:
1. Check wiring connections (+ to D1, - to GND)
2. Verify buzzer polarity (try reversing if passive)
3. Test with different pin (change to D2 in code)
4. Try a different buzzer (it might be damaged)
5. Check if using active vs passive buzzer

### Buzzer Too Loud
**Solutions**:
1. Add 100Ω-220Ω resistor in series with buzzer
2. Reduce frequency values in code patterns
3. Use software PWM to control volume

### Notifications Not Triggering
**Problem**: Web app shows alert but ESP8266 doesn't buzz

**Solutions**:
1. Check User ID matches exactly
2. Check Serial Monitor for "NEW NOTIFICATION RECEIVED!"
3. Verify Firebase Database Rules allow read/write
4. Ensure Firebase path is correct: `/notifications/USER_ID/latest`

---

## 🔒 Firebase Security Rules

Add these rules to allow ESP8266 access:

```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    },
    "notifications": {
      "$uid": {
        ".read": true,
        ".write": "$uid === auth.uid"
      }
    },
    "bloodSugar": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    },
    "medications": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    },
    "medicationIntake": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    },
    "reminders": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    }
  }
}
```

**Important**: The notifications path has `.read: true` to allow ESP8266 to read without authentication.

---

## 📊 Data Structure

Firebase stores notifications at:
```
/notifications/
  └─ {USER_ID}/
      ├─ latest/              ← ESP8266 monitors this
      │   ├─ id
      │   ├─ type
      │   ├─ message
      │   ├─ timestamp
      │   └─ sent
      │
      ├─ history/             ← All notifications stored here
      │   └─ {NOTIFICATION_ID}/
      │       ├─ id
      │       ├─ type
      │       ├─ message
      │       └─ timestamp
      │
      └─ acknowledged/        ← ESP8266 writes here when buzzer plays
          └─ {NOTIFICATION_ID}/
              ├─ acknowledgedAt
              └─ device
```

---

## 🎯 Advanced Customization

### Change Buzzer Pin
In the Arduino code, modify:
```cpp
#define BUZZER_PIN D2  // Change to D2, D5, D6, etc.
```

### Adjust Check Interval
```cpp
const unsigned long CHECK_INTERVAL = 3000; // Check every 3 seconds
```

### Create Custom Buzzer Pattern
```cpp
const int MY_PATTERN[][3] = {
  {frequency, duration, pause},
  {2000, 300, 100},  // 2000Hz for 300ms, pause 100ms
  {2500, 200, 0}     // 2500Hz for 200ms, no pause
};
```

### Multiple ESP8266 Devices
Each device can monitor the same user's notifications. Just upload the same code with the same USER_ID to multiple ESP8266 boards.

---

## 💡 Pro Tips

1. **Power Supply**: For permanent installation, use a 5V USB power adapter
2. **Enclosure**: Put ESP8266 in a plastic case to prevent short circuits
3. **Volume Control**: Add a potentiometer for adjustable volume
4. **Multiple Buzzers**: Connect buzzers to different pins for different notification types
5. **Visual Indicator**: Built-in LED blinks during notifications
6. **Battery Power**: Use power bank for portable notification device
7. **Sound Quality**: Passive buzzers give better tone control than active buzzers

---

## 📱 Mobile Notifications + Buzzer

You can have BOTH web notifications AND physical buzzer:
- Web app shows on-screen alerts
- ESP8266 buzzer provides audible alerts
- Perfect for night-time or when away from computer

---

## 🆘 Support

If you encounter issues:
1. Check Serial Monitor output for error messages
2. Verify all configuration values are correct
3. Test WiFi connection separately
4. Test buzzer with simple tone() commands
5. Check Firebase Console for data updates

---

## ✅ Quick Setup Checklist

- [ ] ESP8266 board purchased
- [ ] Piezo buzzer purchased
- [ ] Arduino IDE installed
- [ ] ESP8266 board support installed
- [ ] FirebaseESP8266 library installed
- [ ] Buzzer wired to D1 and GND
- [ ] WiFi credentials configured in code
- [ ] Firebase credentials configured
- [ ] User ID configured
- [ ] Code uploaded successfully
- [ ] Serial monitor shows "WiFi Connected"
- [ ] Welcome beep plays on startup
- [ ] Test notification triggers buzzer

---

**Your ESP8266 notification system is now ready! Every notification from your DiaWeb app will trigger the buzzer to alert you.**
