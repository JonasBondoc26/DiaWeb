#include <FB_Const.h>
#include <FB_Error.h>
#include <FB_Network.h>
#include <FB_Utils.h>
#include <Firebase.h>
#include <FirebaseFS.h>
#include <Firebase_ESP_Client.h>

/*
 * ESP8266 Notification Buzzer
 * Updated for Firebase ESP Client Library
 * 
 * Hardware Requirements:
 * - ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
 * - Piezo Buzzer
 * 
 * Connections:
 * - Buzzer Positive (+) -> D1 (GPIO5)
 * - Buzzer Negative (-) -> GND
 */

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
#define WIFI_SSID " "      // Your WiFi network name
#define WIFI_PASSWORD " "  // Your WiFi password

// Firebase Credentials
#define API_KEY "AIzaSyC7XlN2DM38tq19LJuT8WyGhcOVcRxfZec"  // From Firebase Project Settings
#define DATABASE_URL "https://diabetic-care-tracker-default-rtdb.asia-southeast1.firebasedatabase.app"  // Include https://
#define USER_EMAIL " "  // Your login email
#define USER_PASSWORD " "  // Your login password

// ========================================
// PIN CONFIGURATION
// ========================================
#define BUZZER_PIN D1        // GPIO5 - Connect buzzer here
#define LED_PIN LED_BUILTIN  // Built-in LED for visual feedback

// ========================================
// BUZZER PATTERNS
// ========================================
// Pattern: {frequency, duration, pause}
// Medication Reminder: 3 short beeps
const int MEDICATION_PATTERN[][3] = {
  {2000, 200, 100},
  {2000, 200, 100},
  {2000, 200, 0}
};

// Custom Reminder: 2 long beeps
const int REMINDER_PATTERN[][3] = {
  {1500, 500, 200},
  {1500, 500, 0}
};

// High Blood Sugar: Continuous beeping
const int HIGH_GLUCOSE_PATTERN[][3] = {
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 100},
  {2500, 150, 0}
};

// Low Blood Sugar: Fast continuous beeping (urgent)
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
const unsigned long CHECK_INTERVAL = 5000; // Check every 5 seconds

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
  digitalWrite(LED_PIN, HIGH); // LED off (inverted logic)
  
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
  unsigned long startTime = millis();
  while (!Firebase.ready() && (millis() - startTime < 30000)) {
    Serial.print(".");
    delay(500);
  }
  
  if (Firebase.ready()) {
    Serial.println();
    Serial.println("Firebase connected!");
    
    // Get user ID from authentication
    userId = auth.token.uid.c_str();
    Serial.print("User ID: ");
    Serial.println(userId);
    
    Serial.println("Setup complete!");
    Serial.println("Monitoring for notifications...");
    Serial.println();
    
    // Welcome beep
    playWelcomeBeep();
  } else {
    Serial.println();
    Serial.println("Firebase connection failed!");
    Serial.println("Check your credentials and try again.");
    Serial.println("Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
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
  
  // Check for new notifications
  if (Firebase.ready() && (millis() - lastCheck >= CHECK_INTERVAL)) {
    lastCheck = millis();
    checkForNotifications();
  }
  
  // Small delay to prevent watchdog issues
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
    digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink LED
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH); // LED off
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
  digitalWrite(LED_PIN, LOW); // LED on during sound
  
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
    // Default pattern for unknown types
    playPattern(REMINDER_PATTERN, 2);
  }
  
  digitalWrite(LED_PIN, HIGH); // LED off
}

// ========================================
// PLAY BUZZER PATTERN
// ========================================
void playPattern(const int pattern[][3], int patternLength) {
  for (int i = 0; i < patternLength; i++) {
    int frequency = pattern[i][0];
    int duration = pattern[i][1];
    int pause = pattern[i][2];
    
    // Play tone
    tone(BUZZER_PIN, frequency, duration);
    delay(duration);
    
    // Pause between tones
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




