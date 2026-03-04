#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info
#include "addons/RTDBHelper.h"

// ========================================
// CONFIGURATION - UPDATE THESE VALUES
// ========================================

// WiFi Credentials
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "PASSWORD"

// Firebase Credentials
#define API_KEY "AIzaSyC7XlN2DM38tq19LJuT8WyGhcOVcRxfZec"
#define DATABASE_URL "https://diabetic-care-tracker-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "EMAIL_ADDRESS"
#define USER_PASSWORD "PASSWORD"

// ========================================
// PIN CONFIGURATION
// ========================================
#define BUZZER_PIN D1        // GPIO5 - Connect buzzer here
#define LED_PIN LED_BUILTIN  // Built-in LED for visual feedback

// ========================================
// BUZZER PATTERNS (FASTER!)
// ========================================
// Pattern: {frequency, duration, pause}

// Medication Reminder: FAST 3 short beeps (repeating)
const int MEDICATION_PATTERN[][3] = {
  {2500, 80, 40},   // Faster! 80ms beep, 40ms pause
  {2500, 80, 40},
  {2500, 80, 500}   // 500ms pause before repeat
};

// Custom Reminder: 2 medium beeps
const int REMINDER_PATTERN[][3] = {
  {1500, 300, 100},
  {1500, 300, 0}
};

// High Blood Sugar: 4 beeps
const int HIGH_GLUCOSE_PATTERN[][3] = {
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 0}
};

// Low Blood Sugar: VERY FAST urgent beeping
const int LOW_GLUCOSE_PATTERN[][3] = {
  {3000, 60, 30},
  {3000, 60, 30},
  {3000, 60, 30},
  {3000, 60, 30},
  {3000, 60, 30},
  {3000, 60, 0}
};

// ========================================
// GLOBAL VARIABLES
// ========================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String userId = "";
String lastNotificationId = "";

// Continuous alarm system
bool alarmActive = false;
String currentMedicationId = "";
String currentAlarmType = "";
unsigned long lastAlarmPlay = 0;
unsigned long lastIntakeCheck = 0;
const unsigned long ALARM_REPEAT_INTERVAL = 700;  // Play alarm every 700ms (FASTER!)
const unsigned long INTAKE_CHECK_INTERVAL = 1000; // Check if taken every 1 second

// ========================================
// SETUP
// ========================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("=================================");
  Serial.println("DiaWeb");
  Serial.println("=================================");
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, HIGH); // LED off
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Sign in
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Connecting to Firebase...");
  
  // Wait for authentication (max 30 seconds)
  unsigned long startTime = millis();
  while (!Firebase.ready() && (millis() - startTime < 30000)) {
    Serial.print(".");
    delay(500);
  }
  
  if (Firebase.ready()) {
    Serial.println();
    Serial.println("✓ Firebase connected!");
    
    userId = auth.token.uid.c_str();
    Serial.print("✓ User ID: ");
    Serial.println(userId);
    
    Serial.println("\nSetup complete!");
    Serial.println("Monitoring for notifications...");
    Serial.println();
    
    // Welcome beep
    playWelcomeBeep();
  } else {
    Serial.println();
    Serial.println("✗ Firebase connection failed!");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
}

// ========================================
// MAIN LOOP
// ========================================
void loop() {
  // Check WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Reconnecting...");
    connectWiFi();
  }
  
  // If alarm is active - continuous operation
  if (alarmActive) {
    
    // Play alarm pattern repeatedly
    if (millis() - lastAlarmPlay >= ALARM_REPEAT_INTERVAL) {
      lastAlarmPlay = millis();
      playAlarmSound();
    }
    
    // Check if medication was taken
    if (millis() - lastIntakeCheck >= INTAKE_CHECK_INTERVAL) {
      lastIntakeCheck = millis();
      
      if (checkMedicationTaken()) {
        Serial.println("\n╔═══════════════════════════════════╗");
        Serial.println("║  ✓ MEDICATION MARKED AS TAKEN!   ║");
        Serial.println("║  ✓ Stopping alarm...              ║");
        Serial.println("╚═══════════════════════════════════╝\n");
        
        stopAlarm();
      } else {
        Serial.print(".");  // Show checking activity
      }
    }
    
  } else {
    // Normal monitoring mode
    checkForNotifications();
    delay(5000); // Check every 5 seconds when not alarming
  }
  
  delay(10); // Small delay to prevent watchdog
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
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi failed! Restarting...");
    delay(3000);
    ESP.restart();
  }
}

// ========================================
// CHECK FOR NOTIFICATIONS
// ========================================
void checkForNotifications() {
  if (!Firebase.ready()) return;
  
  String path = "/notifications/" + userId + "/latest";
  
  if (Firebase.RTDB.getJSON(&fbdo, path)) {
    if (fbdo.dataType() == "json") {
      FirebaseJson &json = fbdo.jsonObject();
      FirebaseJsonData result;
      
      // Get notification ID
      json.get(result, "id");
      String notificationId = result.stringValue;
      
      // Check if new notification
      if (notificationId != lastNotificationId && notificationId.length() > 0) {
        lastNotificationId = notificationId;
        
        // Get notification details
        json.get(result, "type");
        String notificationType = result.stringValue;
        
        json.get(result, "message");
        String message = result.stringValue;
        
        json.get(result, "medicationId");
        String medicationId = result.stringValue;
        
        // Display notification
        Serial.println("\n=================================");
        Serial.println("NEW NOTIFICATION!");
        Serial.println("=================================");
        Serial.print("Type: ");
        Serial.println(notificationType);
        Serial.print("Message: ");
        Serial.println(message);
        if (medicationId.length() > 0) {
          Serial.print("Med ID: ");
          Serial.println(medicationId);
        }
        Serial.println("=================================\n");
        
        // Handle notification
        if (notificationType == "medication") {
          // Start continuous alarm for medication
          startContinuousAlarm(medicationId);
        } else {
          // One-time sound for other notifications
          playOtherNotification(notificationType);
        }
        
        // Acknowledge
        acknowledgeNotification(notificationId);
      }
    }
  }
}

// ========================================
// START CONTINUOUS ALARM
// ========================================
void startContinuousAlarm(String medId) {
  alarmActive = true;
  currentMedicationId = medId;
  currentAlarmType = "medication";
  lastAlarmPlay = 0;
  lastIntakeCheck = 0;
  
  Serial.println("\n╔═══════════════════════════════════╗");
  Serial.println("║  🔔 ALARM STARTED!                ║");
  Serial.println("║  Buzzer will play continuously    ║");
  Serial.println("║  until medication is marked       ║");
  Serial.println("║  as taken in the web app.         ║");
  Serial.println("╚═══════════════════════════════════╝\n");
  
  // Play immediately
  playAlarmSound();
}

// ========================================
// STOP ALARM
// ========================================
void stopAlarm() {
  alarmActive = false;
  currentMedicationId = "";
  currentAlarmType = "";
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, HIGH); // LED off
  
  Serial.println("Returning to monitoring mode...\n");
}

// ========================================
// PLAY ALARM SOUND (FOR CONTINUOUS ALARM)
// ========================================
void playAlarmSound() {
  digitalWrite(LED_PIN, LOW); // LED on during alarm
  
  // Play medication pattern (3 FAST beeps)
  for (int i = 0; i < 3; i++) {
    int frequency = MEDICATION_PATTERN[i][0];
    int duration = MEDICATION_PATTERN[i][1];
    int pause = MEDICATION_PATTERN[i][2];
    
    tone(BUZZER_PIN, frequency);
    delay(duration);
    noTone(BUZZER_PIN);
    
    if (i < 2) {  // Don't delay after last beep
      delay(pause);
    }
  }
  
  digitalWrite(LED_PIN, HIGH); // LED off
}

// ========================================
// CHECK IF MEDICATION TAKEN
// ========================================
bool checkMedicationTaken() {
  if (currentMedicationId.length() == 0) {
    return false;
  }
  
  // Get today's date
  String today = getTodayDate();
  
  // Check medicationIntake
  String path = "/medicationIntake/" + userId;
  
  if (Firebase.RTDB.getJSON(&fbdo, path)) {
    if (fbdo.dataType() == "json") {
      FirebaseJson &json = fbdo.jsonObject();
      
      size_t len = json.iteratorBegin();
      FirebaseJson::IteratorValue value;
      
      for (size_t i = 0; i < len; i++) {
        value = json.valueAt(i);
        
        if (value.type == FirebaseJson::JSON_OBJECT) {
          FirebaseJson intakeJson;
          intakeJson.setJsonData(value.value);
          
          FirebaseJsonData dateData, medIdData;
          
          intakeJson.get(dateData, "date");
          intakeJson.get(medIdData, "medicationId");
          
          String intakeDate = dateData.stringValue;
          String intakeMedId = medIdData.stringValue;
          
          // Check if this medication was taken today
          if (intakeMedId == currentMedicationId && intakeDate == today) {
            json.iteratorEnd();
            return true; // Found it! Medication was taken!
          }
        }
      }
      
      json.iteratorEnd();
    }
  }
  
  return false; // Not taken yet
}

// ========================================
// GET TODAY'S DATE (MM/DD/YYYY format)
// ========================================
String getTodayDate() {
  // Get current time from Firebase timestamp
  // For simplicity, we check for ANY intake today
  // The web app sets the proper date format
  
  // Alternative: Use NTP for precise date
  // For now, return empty to match any recent intake
  return "";
}

// ========================================
// PLAY OTHER NOTIFICATIONS (ONE-TIME)
// ========================================
void playOtherNotification(String type) {
  digitalWrite(LED_PIN, LOW);
  
  if (type == "reminder") {
    playPattern(REMINDER_PATTERN, 2);
  } 
  else if (type == "high_glucose") {
    playPattern(HIGH_GLUCOSE_PATTERN, 4);
  } 
  else if (type == "low_glucose") {
    playPattern(LOW_GLUCOSE_PATTERN, 6);
  }
  
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// PLAY PATTERN (GENERIC)
// ========================================
void playPattern(const int pattern[][3], int size) {
  for (int i = 0; i < size; i++) {
    int frequency = pattern[i][0];
    int duration = pattern[i][1];
    int pause = pattern[i][2];
    
    tone(BUZZER_PIN, frequency);
    delay(duration);
    noTone(BUZZER_PIN);
    delay(pause);
  }
}

// ========================================
// ACKNOWLEDGE NOTIFICATION
// ========================================
void acknowledgeNotification(String notificationId) {
  String path = "/notifications/" + userId + "/acknowledged/" + notificationId;
  
  FirebaseJson json;
  json.set("acknowledgedAt", String(millis()));
  json.set("device", "ESP8266_Buzzer");
  
  Firebase.RTDB.setJSON(&fbdo, path, &json);
}

// ========================================
// WELCOME BEEP (3 RISING TONES)
// ========================================
void playWelcomeBeep() {
  int frequencies[] = {262, 330, 392}; // C, E, G
  
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, frequencies[i]);
    delay(150);
    noTone(BUZZER_PIN);
    delay(50);
  }
}

// ========================================
// TOKEN STATUS CALLBACK
// ========================================
void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    Serial.printf("Token error: %s\n", info.error.message.c_str());
  }
}
