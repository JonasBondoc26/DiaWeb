#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ========================================
// CONFIGURATION
// ========================================
#define WIFI_SSID "WIFI_SSID_HERE"  // REPLACE with actual SSID
#define WIFI_PASSWORD "WIFI_PASSWORD_HERE"  // REPLACE with actual password
#define API_KEY "AIzaSyC7XlN2DM38tq19LJuT8WyGhcOVcRxfZec"
#define DATABASE_URL "https://diabetic-care-tracker-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "USER_EMAIL_HERE"  // REPLACE with actual email
#define USER_PASSWORD "USER_PASSWORD_HERE"  // REPLACE with actual password

#define BUZZER_PIN D1
#define LED_PIN LED_BUILTIN

// ========================================
// BUZZER PATTERNS (ALL TYPES!)
// ========================================
// Medication: FAST 3 short beeps (for continuous alarm)
const int MEDICATION_PATTERN[][3] = {
  {2500, 80, 40},   // Fast!
  {2500, 80, 40},
  {2500, 80, 500}   // Pause before repeat
};

// Reminder: 2 long beeps (one-time)
const int REMINDER_PATTERN[][3] = {
  {1500, 400, 150},
  {1500, 400, 0}
};

// High Glucose: 4 medium beeps (one-time)
const int HIGH_GLUCOSE_PATTERN[][3] = {
  {2500, 200, 100},
  {2500, 200, 100},
  {2500, 200, 100},
  {2500, 200, 0}
};

// Low Glucose: 6 FAST urgent beeps (one-time)
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

// Continuous alarm (for medication only)
bool alarmActive = false;
unsigned long lastAlarmPlay = 0;
unsigned long lastFlagCheck = 0;
const unsigned long ALARM_REPEAT_INTERVAL = 700;
const unsigned long FLAG_CHECK_INTERVAL = 500;

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════╗");
  Serial.println("║  DiaWeb - ESP8266 Buzzer Device    ║");
  Serial.println("╚════════════════════════════════════╝");
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, HIGH);
  
  connectWiFi();
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("\n[SETUP] Connecting to Firebase...");
  
  unsigned long startTime = millis();
  while (!Firebase.ready() && (millis() - startTime < 30000)) {
    Serial.print(".");
    delay(500);
  }
  
  if (Firebase.ready()) {
    Serial.println("\n[SETUP] ✓ Firebase connected!");
    userId = auth.token.uid.c_str();
    Serial.print("[SETUP] ✓ User ID: ");
    Serial.println(userId);
    
    // Clear any old alarm flags
    clearAlarmFlag();
    
    Serial.println("\n[SETUP] ✓ Ready for ALL notification types:");
    Serial.println("  - Medication (continuous alarm)");
    Serial.println("  - Reminder (2 beeps)");
    Serial.println("  - High Glucose (4 beeps)");
    Serial.println("  - Low Glucose (6 fast beeps)");
    Serial.println();
    
    playWelcomeBeep();
  } else {
    Serial.println("\n[SETUP] ✗ Firebase failed! Restarting...");
    delay(10000);
    ESP.restart();
  }
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] Reconnecting...");
    connectWiFi();
  }
  
  if (alarmActive) {
    // MEDICATION CONTINUOUS ALARM MODE
    
    // Play alarm repeatedly
    if (millis() - lastAlarmPlay >= ALARM_REPEAT_INTERVAL) {
      lastAlarmPlay = millis();
      playMedicationAlarm();
    }
    
    // Check if cleared
    if (millis() - lastFlagCheck >= FLAG_CHECK_INTERVAL) {
      lastFlagCheck = millis();
      
      if (checkAlarmFlag()) {
        Serial.println("\n[ALARM] ✓ Medication was taken! Stopping...\n");
        stopAlarm();
      }
    }
  } else {
    // NORMAL MONITORING MODE
    checkForNotifications();
    delay(5000);
  }
  
  delay(10);
}

// ========================================
// WIFI
// ========================================
void connectWiFi() {
  Serial.print("[WIFI] Connecting to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] ✓ Connected!");
    Serial.print("[WIFI] ✓ IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WIFI] ✗ Failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
}

// ========================================
// CHECK NOTIFICATIONS
// ========================================
void checkForNotifications() {
  if (!Firebase.ready()) return;
  
  String path = "/notifications/" + userId + "/latest";
  
  if (Firebase.RTDB.getJSON(&fbdo, path)) {
    if (fbdo.dataType() == "json") {
      FirebaseJson &json = fbdo.jsonObject();
      FirebaseJsonData result;
      
      json.get(result, "id");
      String notificationId = result.stringValue;
      
      if (notificationId != lastNotificationId && notificationId.length() > 0) {
        lastNotificationId = notificationId;
        
        json.get(result, "type");
        String notificationType = result.stringValue;
        
        json.get(result, "message");
        String message = result.stringValue;
        
        // Show notification
        Serial.println("\n╔════════════════════════════════════╗");
        Serial.println("║  NEW NOTIFICATION!                ║");
        Serial.println("╚════════════════════════════════════╝");
        Serial.print("[NOTIF] Type: ");
        Serial.println(notificationType);
        Serial.print("[NOTIF] Message: ");
        Serial.println(message);
        
        // Handle different notification types
        if (notificationType == "medication") {
          Serial.println("[NOTIF] → Starting CONTINUOUS alarm");
          startMedicationAlarm();
        } 
        else if (notificationType == "reminder") {
          Serial.println("[NOTIF] → Playing REMINDER sound (2 beeps)");
          playReminderSound();
        } 
        else if (notificationType == "high_glucose") {
          Serial.println("[NOTIF] → Playing HIGH GLUCOSE alert (4 beeps)");
          playHighGlucoseSound();
        } 
        else if (notificationType == "low_glucose") {
          Serial.println("[NOTIF] → Playing LOW GLUCOSE alert (6 fast beeps)");
          playLowGlucoseSound();
        }
        else {
          Serial.println("[NOTIF] → Unknown type, playing default sound");
          playReminderSound();
        }
        
        acknowledgeNotification(notificationId);
        Serial.println();
      }
    }
  }
}

// ========================================
// START MEDICATION ALARM (CONTINUOUS)
// ========================================
void startMedicationAlarm() {
  alarmActive = true;
  lastAlarmPlay = 0;
  lastFlagCheck = 0;
  
  // Set flag
  String flagPath = "/alarmState/" + userId;
  FirebaseJson flagJson;
  flagJson.set("active", true);
  flagJson.set("timestamp", String(millis()));
  Firebase.RTDB.setJSON(&fbdo, flagPath, &flagJson);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  🔔 MEDICATION ALARM ACTIVE!      ║");
  Serial.println("║  Continuous until marked as taken ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  playMedicationAlarm();
}

// ========================================
// STOP ALARM
// ========================================
void stopAlarm() {
  alarmActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("[ALARM] Stopped. Back to monitoring mode.\n");
}

// ========================================
// CHECK ALARM FLAG
// ========================================
bool checkAlarmFlag() {
  String flagPath = "/alarmState/" + userId + "/active";
  
  if (Firebase.RTDB.getBool(&fbdo, flagPath)) {
    bool flagValue = fbdo.boolData();
    
    if (!flagValue) {
      return true;  // Flag is FALSE = stop alarm!
    }
  } else {
    String error = fbdo.errorReason();
    if (error.indexOf("not exist") >= 0 || error.indexOf("null") >= 0) {
      return true;  // Flag deleted = stop alarm!
    }
  }
  
  return false;
}

// ========================================
// CLEAR FLAG
// ========================================
void clearAlarmFlag() {
  Serial.println("[SETUP] Clearing old alarm flags...");
  
  String flagPath = "/alarmState/" + userId;
  FirebaseJson flagJson;
  flagJson.set("active", false);
  flagJson.set("clearedAt", "startup");
  
  Firebase.RTDB.setJSON(&fbdo, flagPath, &flagJson);
  Serial.println("[SETUP] ✓ Flags cleared");
}

// ========================================
// PLAY SOUNDS (ONE-TIME)
// ========================================
void playReminderSound() {
  digitalWrite(LED_PIN, LOW);
  playPattern(REMINDER_PATTERN, 2);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("[BUZZER] ✓ Reminder sound played (2 beeps)");
}

void playHighGlucoseSound() {
  digitalWrite(LED_PIN, LOW);
  playPattern(HIGH_GLUCOSE_PATTERN, 4);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("[BUZZER] ✓ High glucose alert played (4 beeps)");
}

void playLowGlucoseSound() {
  digitalWrite(LED_PIN, LOW);
  playPattern(LOW_GLUCOSE_PATTERN, 6);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("[BUZZER] ✓ Low glucose alert played (6 fast beeps)");
}

// ========================================
// PLAY MEDICATION ALARM (CONTINUOUS)
// ========================================
void playMedicationAlarm() {
  digitalWrite(LED_PIN, LOW);
  playPattern(MEDICATION_PATTERN, 3);
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// PLAY PATTERN (HELPER)
// ========================================
void playPattern(const int pattern[][3], int size) {
  for (int i = 0; i < size; i++) {
    tone(BUZZER_PIN, pattern[i][0]);
    delay(pattern[i][1]);
    noTone(BUZZER_PIN);
    if (pattern[i][2] > 0) {
      delay(pattern[i][2]);
    }
  }
  noTone(BUZZER_PIN);
}

// ========================================
// ACKNOWLEDGE
// ========================================
void acknowledgeNotification(String notificationId) {
  String path = "/notifications/" + userId + "/acknowledged/" + notificationId;
  FirebaseJson json;
  json.set("acknowledgedAt", String(millis()));
  json.set("device", "ESP8266_Buzzer");
  Firebase.RTDB.setJSON(&fbdo, path, &json);
}

// ========================================
// WELCOME BEEP
// ========================================
void playWelcomeBeep() {
  Serial.println("[BUZZER] Playing welcome beep...");
  int frequencies[] = {262, 330, 392};
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, frequencies[i]);
    delay(150);
    noTone(BUZZER_PIN);
    delay(50);
  }
  Serial.println("[BUZZER] ✓ Welcome beep done\n");
}

// ========================================
// TOKEN CALLBACK
// ========================================
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    Serial.printf("[TOKEN] Error: %s\n", info.error.message.c_str());
  }
}
