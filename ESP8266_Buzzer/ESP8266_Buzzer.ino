#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ========================================
// CONFIGURATION
// ========================================
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
#define API_KEY "AIzaSyC7XlN2DM38tq19LJuT8WyGhcOVcRxfZec"
#define DATABASE_URL "https://diabetic-care-tracker-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

#define BUZZER_PIN D1
#define LED_PIN LED_BUILTIN

// ========================================
// BUZZER PATTERNS
// ========================================
const int MEDICATION_PATTERN[][3] = {
  {2500, 80, 40},
  {2500, 80, 40},
  {2500, 80, 500}
};

// ========================================
// GLOBAL VARIABLES
// ========================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String userId = "";
String lastNotificationId = "";

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
  Serial.println("║  DiaWeb  ");
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
    
    // IMPORTANT: Clear any old alarm flags on startup
    clearAlarmFlag();
    
    Serial.println("\n[SETUP] Setup complete! Monitoring...\n");
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
    Serial.println("[LOOP] WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
  
  if (alarmActive) {
    // Play alarm
    if (millis() - lastAlarmPlay >= ALARM_REPEAT_INTERVAL) {
      lastAlarmPlay = millis();
      playAlarmSound();
    }
    
    // Check flag
    if (millis() - lastFlagCheck >= FLAG_CHECK_INTERVAL) {
      lastFlagCheck = millis();
      
      Serial.print("[ALARM] Checking flag... ");
      
      if (checkAlarmFlag()) {
        Serial.println("FLAG CLEARED!");
        Serial.println("\n╔════════════════════════════════════╗");
        Serial.println("║  ✓ ALARM STOPPED!                 ║");
        Serial.println("║  Medication was marked as taken   ║");
        Serial.println("╚════════════════════════════════════╝\n");
        stopAlarm();
      } else {
        Serial.println("still active");
      }
    }
  } else {
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
        
        Serial.println("\n╔════════════════════════════════════╗");
        Serial.println("║  NEW NOTIFICATION!                ║");
        Serial.println("╚════════════════════════════════════╝");
        Serial.print("[NOTIF] Type: ");
        Serial.println(notificationType);
        Serial.print("[NOTIF] Message: ");
        Serial.println(message);
        Serial.print("[NOTIF] ID: ");
        Serial.println(notificationId);
        
        if (notificationType == "medication") {
          startContinuousAlarm();
        }
        
        acknowledgeNotification(notificationId);
      }
    }
  }
}

// ========================================
// START ALARM
// ========================================
void startContinuousAlarm() {
  alarmActive = true;
  lastAlarmPlay = 0;
  lastFlagCheck = 0;
  
  Serial.println("\n[ALARM] Setting alarm flag in Firebase...");
  
  // Set flag to TRUE
  String flagPath = "/alarmState/" + userId;
  FirebaseJson flagJson;
  flagJson.set("active", true);
  flagJson.set("timestamp", String(millis()));
  
  if (Firebase.RTDB.setJSON(&fbdo, flagPath, &flagJson)) {
    Serial.println("[ALARM] ✓ Flag SET to TRUE in Firebase");
  } else {
    Serial.print("[ALARM] ✗ Failed to set flag: ");
    Serial.println(fbdo.errorReason());
  }
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  ALARM STARTED!                ║");
  Serial.println("║  Checking flag every 500ms...     ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  playAlarmSound();
}

// ========================================
// STOP ALARM
// ========================================
void stopAlarm() {
  alarmActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.println("[ALARM] Alarm stopped. Monitoring mode...\n");
}

// ========================================
// CHECK FLAG (WITH DEBUG)
// ========================================
bool checkAlarmFlag() {
  String flagPath = "/alarmState/" + userId + "/active";
  
  Serial.print("[FLAG] Reading: ");
  Serial.print(flagPath);
  Serial.print(" → ");
  
  if (Firebase.RTDB.getBool(&fbdo, flagPath)) {
    bool flagValue = fbdo.boolData();
    
    Serial.print("Value: ");
    Serial.print(flagValue ? "TRUE" : "FALSE");
    Serial.print(" → ");
    
    if (!flagValue) {
      Serial.println("STOP!");
      return true;  // Stop alarm!
    } else {
      Serial.println("Continue");
      return false; // Continue alarm
    }
  } else {
    Serial.print("ERROR: ");
    Serial.println(fbdo.errorReason());
    
    // If error contains "not exist" or "null", stop alarm
    String error = fbdo.errorReason();
    if (error.indexOf("not exist") >= 0 || error.indexOf("null") >= 0) {
      Serial.println("[FLAG] Flag doesn't exist → STOP!");
      return true;
    }
    
    return false;
  }
}

// ========================================
// CLEAR FLAG ON STARTUP
// ========================================
void clearAlarmFlag() {
  Serial.println("[SETUP] Clearing any old alarm flags...");
  
  String flagPath = "/alarmState/" + userId;
  FirebaseJson flagJson;
  flagJson.set("active", false);
  flagJson.set("clearedAt", "startup");
  
  if (Firebase.RTDB.setJSON(&fbdo, flagPath, &flagJson)) {
    Serial.println("[SETUP] ✓ Old flags cleared");
  } else {
    Serial.print("[SETUP] Note: ");
    Serial.println(fbdo.errorReason());
  }
}

// ========================================
// PLAY ALARM
// ========================================
void playAlarmSound() {
  digitalWrite(LED_PIN, LOW);
  
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, MEDICATION_PATTERN[i][0]);
    delay(MEDICATION_PATTERN[i][1]);
    noTone(BUZZER_PIN);
    if (i < 2) delay(MEDICATION_PATTERN[i][2]);
  }
  
  digitalWrite(LED_PIN, HIGH);
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
  int frequencies[] = {262, 330, 392};
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, frequencies[i]);
    delay(150);
    noTone(BUZZER_PIN);
    delay(50);
  }
}
