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
#define WIFI_SSID "YOUR_WIFI_SSID"      
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Credentials
#define API_KEY "YOUR_API_KEY"  
#define DATABASE_URL "YOUR_DATABASE_URL"  
#define USER_EMAIL "your@email.com"  
#define USER_PASSWORD "yourpassword"  

// ========================================
// PIN CONFIGURATION
// ========================================
#define BUZZER_PIN D1        
#define LED_PIN LED_BUILTIN  
#define BUTTON_PIN D2        

// ========================================
// BUZZER PATTERNS & ALARM SETTINGS
// ========================================
// Pattern: {frequency, duration, pause}

// Medication Reminder: FASTER 3 short beeps (repeating)
const int MEDICATION_PATTERN[][3] = {
  {2500, 100, 50},
  {2500, 100, 50},
  {2500, 100, 300}
};

// Custom Reminder
const int REMINDER_PATTERN[][3] = {
  {1500, 300, 100},
  {1500, 300, 500}
};

// High Blood Sugar
const int HIGH_GLUCOSE_PATTERN[][3] = {
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 500}
};

// Low Blood Sugar
const int LOW_GLUCOSE_PATTERN[][3] = {
  {3000, 80, 40},
  {3000, 80, 40},
  {3000, 80, 40},
  {3000, 80, 40},
  {3000, 80, 40},
  {3000, 80, 300}
};

// ========================================
// GLOBAL VARIABLES
// ========================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String userId = "";
String lastNotificationId = "";
String currentAlarmType = "";
String currentMedicationId = "";
bool alarmActive = false;
unsigned long lastAlarmCheck = 0;
unsigned long alarmCheckInterval = 500; // Check every 0.5 seconds during alarm (FASTER)
String alarmStartTimestamp = ""; // ISO timestamp when alarm started

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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, HIGH); 
  
  // Play welcome beep
  playWelcomeBeep();
  
  // Connect to WiFi
  connectWiFi();
  
  // Configure Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Set user credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Assign the callback function for token generation task
  config.token_status_callback = tokenStatusCallback;
  
  // Begin Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Wait for authentication
  Serial.println("Authenticating with Firebase...");
  while (!Firebase.ready()) {
    delay(100);
  }
  
  Serial.println("Firebase connected!");
  
  // Get User ID from auth
  userId = auth.token.uid.c_str();
  Serial.print("User ID: ");
  Serial.println(userId);
  
  Serial.println("\nSetup complete!");
  Serial.println("Monitoring for notifications...");
  Serial.println();
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
  
  // If alarm is active, check if medication was taken
  if (alarmActive) {

    if (millis() - lastAlarmCheck >= alarmCheckInterval) {
      lastAlarmCheck = millis();
      
      if (checkMedicationTaken()) {
        Serial.println("\n=================================");
        Serial.println("MEDICATION MARKED AS TAKEN!");
        Serial.println("Stopping alarm...");
        Serial.println("=================================\n");
        
        stopAlarm();
      } else {
        playAlarmPattern();
      }
      
      // Optional: Button to stop alarm manually
      if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("\n=================================");
        Serial.println("BUTTON PRESSED - Stopping alarm");
        Serial.println("=================================\n");
        stopAlarm();
        delay(300);
      }
    }
    
  } else {
    // Normal operation - check for new notifications every 5 seconds
    checkForNotifications();
    delay(5000);
  }
}

// ========================================
// WIFI CONNECTION
// ========================================
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
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
      
      json.get(result, "id");
      String notificationId = result.stringValue;
      
      if (notificationId != lastNotificationId && notificationId.length() > 0) {

        lastNotificationId = notificationId;
        
        json.get(result, "type");
        String notificationType = result.stringValue;
        
        json.get(result, "medicationId");
        String medicationId = result.stringValue;
        
        Serial.println("=================================");
        Serial.println("NEW NOTIFICATION RECEIVED!");
        Serial.println("=================================");
        Serial.print("Type: ");
        Serial.println(notificationType);
        Serial.println("=================================");
        
        if (notificationType == "medication") {
          startAlarm(notificationType, medicationId);
        } else {
          playNotificationSound(notificationType);
        }
        
        acknowledgeNotification(notificationId);
      }
    }
  }
}

// ========================================
// START CONTINUOUS ALARM
// ========================================
void startAlarm(String type, String medId) {
  alarmActive = true;
  currentAlarmType = type;
  currentMedicationId = medId;
  lastAlarmCheck = millis();
  
  // Record alarm start timestamp so web app can tag intake records
  // We use a simple counter since ESP has no real-time clock
  alarmStartTimestamp = String(millis());
  
  // Write alarm state to Firebase so web app knows alarm is active
  String alarmPath = "/alarmState/" + userId;
  FirebaseJson alarmJson;
  alarmJson.set("active", true);
  alarmJson.set("medicationId", medId);
  alarmJson.set("alarmToken", alarmStartTimestamp);
  Firebase.RTDB.setJSON(&fbdo, alarmPath, &alarmJson);
  
  Serial.println("\nALARM STARTED - CONTINUOUS\n");
  
  playAlarmPattern();
}

// ========================================
// STOP ALARM
// ========================================
void stopAlarm() {
  alarmActive = false;
  currentAlarmType = "";
  currentMedicationId = "";
  alarmStartTimestamp = "";
  noTone(BUZZER_PIN);
  digitalWrite(LED_PIN, HIGH);
  
  // Clear alarm state in Firebase
  String alarmPath = "/alarmState/" + userId;
  FirebaseJson alarmJson;
  alarmJson.set("active", false);
  Firebase.RTDB.setJSON(&fbdo, alarmPath, &alarmJson);
  
  Serial.println("Alarm stopped. Returning to monitoring mode...\n");
}

// ========================================
// PLAY ALARM PATTERN (FASTER, REPEATING)
// ========================================
void playAlarmPattern() {
  digitalWrite(LED_PIN, LOW);
  
  if (currentAlarmType == "medication") {
    playPattern(MEDICATION_PATTERN, 3);
  }
  
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// CHECK IF MEDICATION WAS TAKEN (for THIS alarm session)
// ========================================
bool checkMedicationTaken() {

  if (currentMedicationId.length() == 0 || alarmStartTimestamp.length() == 0) {
    return false;
  }
  
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

          FirebaseJsonData medIdData;
          intakeJson.get(medIdData, "medicationId");
          String intakeMedId = medIdData.stringValue;

          if (intakeMedId == currentMedicationId) {
            // Only count this intake if it was tagged with our alarmToken
            // (meaning the user pressed "Mark as Taken" while THIS alarm was active)
            FirebaseJsonData tokenData;
            intakeJson.get(tokenData, "alarmToken");
            
            if (tokenData.stringValue == alarmStartTimestamp) {
              json.iteratorEnd();
              return true;
            }
          }
        }
      }

      json.iteratorEnd();
    }
  }
  
  return false;
}

// ========================================
// PLAY NOTIFICATION SOUND (ONE-TIME)
// ========================================
void playNotificationSound(String type) {
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
  else {
    playPattern(MEDICATION_PATTERN, 3);
  }
  
  digitalWrite(LED_PIN, HIGH);
}

// ========================================
// PLAY PATTERN
// ========================================
void playPattern(const int pattern[][3], int patternSize) {

  for (int i = 0; i < patternSize; i++) {

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