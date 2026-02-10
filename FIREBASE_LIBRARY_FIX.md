# 🔧 Fix: FirebaseESP8266 Library Not Found

## Solution 1: Install Firebase Arduino Client Library (Recommended)

### Step-by-Step Installation

1. **Open Arduino IDE**

2. **Go to Library Manager**
   - Click: `Sketch` → `Include Library` → `Manage Libraries...`
   - Or press: `Ctrl+Shift+I` (Windows/Linux) or `Cmd+Shift+I` (Mac)

3. **Search for Firebase**
   - In the search box, type: **`Firebase ESP Client`**
   - Look for: **"Firebase Arduino Client Library for ESP8266 and ESP32"**
   - Author: **Mobizt**

4. **Install the Library**
   - Click on the library
   - Click **"Install"** button
   - Wait for installation to complete (may take 1-2 minutes)
   - You should see "INSTALLED" when done

5. **Close and Reopen Arduino IDE**
   - This ensures the library is properly loaded

---

## Solution 2: Manual Installation (If Solution 1 Doesn't Work)

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
/*
 * ESP8266 DiaWeb Notification Buzzer
 * Updated for Firebase ESP Client Library
 */

#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions
#include "addons/RTDBHelper.h"

// ========================================
// CONFIGURATION - UPDATE THESE VALUES
// ========================================

// WiFi Credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Credentials
#define API_KEY "YOUR_FIREBASE_API_KEY"  // From Firebase Project Settings
#define DATABASE_URL "https://YOUR_PROJECT.firebaseio.com"  // Include https://
#define USER_EMAIL "your-email@example.com"  // Your login email
#define USER_PASSWORD "your-password"  // Your login password

// ========================================
// PIN CONFIGURATION
// ========================================
#define BUZZER_PIN D1        // GPIO5 - Connect buzzer here
#define LED_PIN LED_BUILTIN  // Built-in LED for visual feedback

// ========================================
// BUZZER PATTERNS
// ========================================
const int MEDICATION_PATTERN[][3] = {
  {2000, 200, 100},
  {2000, 200, 100},
  {2000, 200, 0}
};

const int REMINDER_PATTERN[][3] = {
  {1500, 500, 200},
  {1500, 500, 0}
};

const int HIGH_GLUCOSE_PATTERN[][3] = {
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 0}
};

const int LOW_GLUCOSE_PATTERN[][3] = {
  {3000, 100, 50},
  {3000, 100, 50},
  {3000, 100, 50},
  {3000, 100, 50},
  {3000, 100, 50},
  {3000, 100, 0}
};

// ========================================
// GLOBAL VARIABLES
// ========================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String userId = "";
String lastNotificationId = "";
unsigned long lastCheck = 0;
const unsigned long CHECK_INTERVAL = 5000;
bool signupOK = false;

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=================================");
  Serial.println("DiaWeb Notification Buzzer");
  Serial.println("=================================");
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Sign in to Firebase
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Assign the callback function for token generation
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Connecting to Firebase...");
  
  // Wait for authentication
  while (!Firebase.ready()) {
    Serial.print(".");
    delay(500);
  }
  
  Serial.println();
  Serial.println("Firebase connected!");
  
  // Get user ID
  userId = auth.token.uid.c_str();
  Serial.print("User ID: ");
  Serial.println(userId);
  
  Serial.println("Setup complete!");
  Serial.println("Monitoring for notifications...");
  Serial.println();
  
  // Welcome beep
  playWelcomeBeep();
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
  
  // Check Firebase is ready
  if (Firebase.ready() && (millis() - lastCheck >= CHECK_INTERVAL)) {
    lastCheck = millis();
    checkForNotifications();
  }
  
  delay(10);
}

// ========================================
// WIFI CONNECTION
// ========================================
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi Connection Failed!");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
}

// ========================================
// CHECK FOR NOTIFICATIONS
// ========================================
void checkForNotifications() {
  String path = "/notifications/" + userId + "/latest";
  
  if (Firebase.RTDB.getJSON(&fbdo, path)) {
    if (fbdo.dataType() == "json") {
      FirebaseJson &json = fbdo.jsonObject();
      FirebaseJsonData result;
      
      // Get notification ID
      json.get(result, "id");
      String notificationId = result.stringValue;
      
      // Check if this is a new notification
      if (notificationId != lastNotificationId && notificationId.length() > 0) {
        lastNotificationId = notificationId;
        
        // Get notification type
        json.get(result, "type");
        String notificationType = result.stringValue;
        
        // Get notification message
        json.get(result, "message");
        String message = result.stringValue;
        
        // Get timestamp
        json.get(result, "timestamp");
        String timestamp = result.stringValue;
        
        // Display notification info
        Serial.println("=================================");
        Serial.println("NEW NOTIFICATION RECEIVED!");
        Serial.println("=================================");
        Serial.print("Type: ");
        Serial.println(notificationType);
        Serial.print("Message: ");
        Serial.println(message);
        Serial.print("Time: ");
        Serial.println(timestamp);
        Serial.println("=================================");
        Serial.println();
        
        // Play appropriate buzzer pattern
        playNotificationSound(notificationType);
        
        // Mark notification as acknowledged
        acknowledgeNotification(notificationId);
      }
    }
  } else {
    Serial.print("Failed to get notifications: ");
    Serial.println(fbdo.errorReason());
  }
}

// ========================================
// PLAY NOTIFICATION SOUND
// ========================================
void playNotificationSound(String type) {
  digitalWrite(LED_PIN, LOW);
  
  if (type == "medication") {
    playPattern(MEDICATION_PATTERN, 3);
  } 
  else if (type == "reminder") {
    playPattern(REMINDER_PATTERN, 2);
  } 
  else if (type == "high_glucose") {
    playPattern(HIGH_GLUCOSE_PATTERN, 4);
  } 
  else if (type == "low_glucose") {
    playPattern(LOW_GLUCOSE_PATTERN, 6);
  } 
  else {
    playPattern(REMINDER_PATTERN, 2);
  }
  
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// PLAY BUZZER PATTERN
// ========================================
void playPattern(const int pattern[][3], int patternLength) {
  for (int i = 0; i < patternLength; i++) {
    int frequency = pattern[i][0];
    int duration = pattern[i][1];
    int pause = pattern[i][2];
    
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    
    if (pause > 0) {
      noTone(BUZZER_PIN);
      delay(pause);
    }
  }
  noTone(BUZZER_PIN);
}

// ========================================
// WELCOME BEEP
// ========================================
void playWelcomeBeep() {
  digitalWrite(LED_PIN, LOW);
  tone(BUZZER_PIN, 1000, 100);
  delay(150);
  tone(BUZZER_PIN, 1500, 100);
  delay(150);
  tone(BUZZER_PIN, 2000, 100);
  delay(150);
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// ACKNOWLEDGE NOTIFICATION
// ========================================
void acknowledgeNotification(String notificationId) {
  String path = "/notifications/" + userId + "/acknowledged/" + notificationId;
  
  FirebaseJson json;
  json.set("acknowledgedAt", String(millis()));
  json.set("device", "ESP8266_Buzzer");
  
  if (Firebase.RTDB.setJSON(&fbdo, path, &json)) {
    Serial.println("Notification acknowledged in Firebase");
  } else {
    Serial.print("Failed to acknowledge: ");
    Serial.println(fbdo.errorReason());
  }
}

// ========================================
// TONE FUNCTIONS
// ========================================
void tone(uint8_t pin, unsigned int frequency, unsigned long duration) {
  unsigned long period = 1000000L / frequency;
  unsigned long elapsed = 0;
  
  while (elapsed < duration * 1000L) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(period / 2);
    digitalWrite(pin, LOW);
    delayMicroseconds(period / 2);
    elapsed += period;
  }
}

void noTone(uint8_t pin) {
  digitalWrite(pin, LOW);
}
```

---

## Getting Your Firebase Credentials

### 1. Get API Key
1. Go to [Firebase Console](https://console.firebase.google.com)
2. Select your project
3. Click **Gear Icon** → **Project Settings**
4. Under "General" tab, find **Web API Key**
5. Copy it (looks like: `AIzaSyXXXXXXXXXXXXXXXXXXXXXXXXXXXXX`)

### 2. Get Database URL
1. In Firebase Console, go to **Realtime Database**
2. Copy the URL shown at top (e.g., `https://your-project.firebaseio.com`)

### 3. User Credentials
Use the same email and password you use to login to your DiaWeb web app.

---

## Configuration Example

```cpp
// WiFi
#define WIFI_SSID "MyHomeWiFi"
#define WIFI_PASSWORD "mypassword123"

// Firebase
#define API_KEY "AIzaSyAbCdEfGhIjKlMnOpQrStUvWxYz1234567"
#define DATABASE_URL "https://diabecare-12345.firebaseio.com"
#define USER_EMAIL "john@example.com"
#define USER_PASSWORD "mypassword"
```

---

## Verify Installation

After installing the library, go to:
- `Sketch` → `Include Library`

You should see:
- ✅ **Firebase Arduino Client Library for ESP8266 and ESP32**

Or in the Library Manager, search "Firebase" and you should see:
- **Firebase ESP Client** by Mobizt (INSTALLED)

---

## If You Still Have Issues

### Alternative: Install Specific Version

1. Open Library Manager
2. Search: **Firebase ESP Client**
3. Click on the library
4. Click the version dropdown
5. Select version **4.3.12** or newer
6. Click Install

### Check Board Selection

Make sure you have:
- **Tools** → **Board** → **ESP8266 Boards** → **NodeMCU 1.0 (ESP-12E Module)**

If you don't see ESP8266 Boards:
1. File → Preferences
2. Additional Board Manager URLs:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Tools → Board → Boards Manager
4. Search "esp8266"
5. Install "esp8266 by ESP8266 Community"

---

## Expected Serial Monitor Output

When everything works, you should see:

```
=================================
DiaWeb Notification Buzzer
=================================
Connecting to WiFi: MyHomeWiFi
........
WiFi Connected!
IP Address: 192.168.1.100
Connecting to Firebase...
.....
Firebase connected!
User ID: abc123xyz456...
Setup complete!
Monitoring for notifications...

```

---

Let me know if you need any more help!
```

Save this as your new Arduino code and it should work perfectly! 🎯
