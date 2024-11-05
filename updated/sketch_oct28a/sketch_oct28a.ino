#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>   // Library for the DS3231 RTC module

// Define pins for RFID
#define RST_PIN 4        // GPIO4 for RFID reset pin
#define SS_PIN 5         // GPIO5 for RFID SS pin (SDA)
#define BUZZER_PIN 12    // GPIO12 for buzzer

// WiFi credentials
#define WIFI_SSID "CUET--Students"
#define WIFI_PASSWORD "1020304050"

// Firebase credentials
#define FIREBASE_HOST "https://rfid-296ab-default-rtdb.firebaseio.com/"
#define FIREBASE_EMAIL "shafqatnawazchy@gmail.com"
#define FIREBASE_PASSWORD "123456"

// Firebase authentication and configuration
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// RFID and RTC instances
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;   // Real-Time Clock instance

// Variables to track status
String lastStatus = "Will Take";

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23);              // Initialize SPI bus with SCK, MISO, MOSI
  mfrc522.PCD_Init();                 // Initialize RFID reader
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Set Firebase configuration
  config.host = FIREBASE_HOST;
  config.api_key = "AIzaSyDApyR0KFpoSQDdimQImVq01sYH4uW9DaE";
  auth.user.email = FIREBASE_EMAIL;
  auth.user.password = FIREBASE_PASSWORD;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Initialize LCD
  Wire.begin(21, 22);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Ready to scan");

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    lcd.clear();
    lcd.print("RTC not found!");
    while (1);  // Stop if RTC is not found
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Set to compile time if RTC lost power
  }

  Serial.println("Place your RFID card near the reader...");
}

void loop() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent()) return;

  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Get UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }

  // Toggle status
  String status = (lastStatus == "Taken") ? "Will Take" : "Taken";
  lastStatus = status;

  // Get current time
  DateTime now = rtc.now();
  String timeStr = String(now.hour()) + ":" + String(now.minute());  // Only hour and minute
  String dateStr = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day());

  // Update Firebase with status and time
  String path = "/Tag_" + uid;
  if (Firebase.setString(firebaseData, path + "/status", status) &&
      Firebase.setString(firebaseData, path + "/time", timeStr) &&
      Firebase.setString(firebaseData, path + "/date", dateStr)) {
    Serial.println("Firebase database updated successfully.");

    // Display UID, status, time, and date on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Med ID: ");
    lcd.print(uid);
    lcd.setCursor(0, 1);
    lcd.print("Med State: ");
    lcd.print(status);
    lcd.setCursor(0, 2);
    lcd.print("Med Time: ");
    lcd.print(timeStr);
    lcd.setCursor(0, 3);
    lcd.print("Date: ");
    lcd.print(dateStr);

    // Activate buzzer
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    Serial.println("Failed to update Firebase: " + firebaseData.errorReason());

    // Display error on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Firebase Error");
    lcd.setCursor(0, 1);
    lcd.print(firebaseData.errorReason().substring(0, 20));
  }
  delay(1000); // Debounce delay
}
