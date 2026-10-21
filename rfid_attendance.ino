/*
  RFID Attendance System
  ESP32 + MFRC522 + 20x4 I2C LCD + buzzer + LED
  Optional cloud logging via Google Apps Script Web App.

  IMPORTANT:
  1. Replace Wi-Fi placeholders below.
  2. Replace example student UIDs/names with your actual registered cards.
  3. Paste your deployed Google Apps Script Web App URL into GOOGLE_SCRIPT_URL.
  4. Verify GPIO assignments against your actual wiring before powering on.
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ---------- Pin configuration: confirm against your wiring ----------
#define RFID_SS_PIN     5
#define RFID_RST_PIN   27
#define SPI_SCK_PIN    18
#define SPI_MISO_PIN   19
#define SPI_MOSI_PIN   23
#define BUZZER_PIN      4
#define LED_PIN         2

// Common I2C pins for many ESP32 DevKit boards.
// Change these if your LCD is wired differently.
#define I2C_SDA_PIN    21
#define I2C_SCL_PIN    22

// ---------- Wi-Fi and Google Sheets configuration ----------
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Deploy your Google Apps Script as a Web App and paste its /exec URL here.
// Leave empty ("") to run locally without cloud logging.
const char* GOOGLE_SCRIPT_URL = "YOUR_GOOGLE_APPS_SCRIPT_WEB_APP_URL";

// ---------- Hardware objects ----------
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ---------- Registered cards: replace sample UIDs ----------
struct Student {
  const char* uid;   // Uppercase UID, bytes separated by colons, e.g. "A1:B2:C3:D4"
  const char* name;
  bool loggedIn;
};

Student students[] = {
  {"AA:BB:CC:DD", "Student One", false},
  {"11:22:33:44", "Student Two", false},
  {"12:34:56:78", "Student Three", false}
};
const size_t STUDENT_COUNT = sizeof(students) / sizeof(students[0]);

String lastScannedUID = "";
unsigned long lastScanMillis = 0;
const unsigned long SCAN_COOLDOWN_MS = 1800;

// ---------- Helpers ----------
void showMessage(const String& line1,
                 const String& line2 = "",
                 const String& line3 = "",
                 const String& line4 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(line1.substring(0, 20));
  lcd.setCursor(0, 1); lcd.print(line2.substring(0, 20));
  lcd.setCursor(0, 2); lcd.print(line3.substring(0, 20));
  lcd.setCursor(0, 3); lcd.print(line4.substring(0, 20));
}

void beepSuccess() {
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 2000, 120);
  delay(160);
  tone(BUZZER_PIN, 2600, 100);
  delay(120);
  digitalWrite(LED_PIN, LOW);
}

void beepError() {
  digitalWrite(LED_PIN, LOW);
  tone(BUZZER_PIN, 500, 450);
  delay(480);
}

String readCardUID() {
  String uid;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) uid += ":";
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

int findStudentByUID(const String& uid) {
  for (size_t i = 0; i < STUDENT_COUNT; i++) {
    if (uid.equalsIgnoreCase(students[i].uid)) return (int)i;
  }
  return -1;
}

String jsonEscape(const String& input) {
  String output;
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    if (c == '"' || c == '\\') {
      output += '\\';
      output += c;
    } else if (c == '\n') {
      output += "\\n";
    } else if (c == '\r') {
      output += "\\r";
    } else {
      output += c;
    }
  }
  return output;
}

bool cloudConfigured() {
  return strlen(GOOGLE_SCRIPT_URL) > 0 &&
         String(GOOGLE_SCRIPT_URL) != "YOUR_GOOGLE_APPS_SCRIPT_WEB_APP_URL";
}

bool sendAttendanceToGoogleSheets(const String& uid,
                                  const String& studentName,
                                  const String& status) {
  if (WiFi.status() != WL_CONNECTED || !cloudConfigured()) {
    Serial.println("Cloud logging skipped: Wi-Fi or URL not configured.");
    return false;
  }

  WiFiClientSecure client;
  // Simplifies initial testing, but does not validate the server certificate.
  // For production, configure certificate validation instead of setInsecure().
  client.setInsecure();

  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.setTimeout(12000);

  if (!https.begin(client, GOOGLE_SCRIPT_URL)) {
    Serial.println("Could not start HTTPS request.");
    return false;
  }

  https.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"uid\":\"" + jsonEscape(uid) + "\",";
  payload += "\"name\":\"" + jsonEscape(studentName) + "\",";
  payload += "\"status\":\"" + jsonEscape(status) + "\"";
  payload += "}";

  int httpCode = https.POST(payload);
  String response = https.getString();

  Serial.print("Google Sheets HTTP code: ");
  Serial.println(httpCode);
  Serial.print("Google Sheets response: ");
  Serial.println(response);

  https.end();
  return httpCode >= 200 && httpCode < 300;
}

void connectWiFi() {
  if (String(WIFI_SSID) == "YOUR_WIFI_NAME" ||
      String(WIFI_PASSWORD) == "YOUR_WIFI_PASSWORD") {
    Serial.println("Wi-Fi credentials are placeholders. Skipping Wi-Fi.");
    showMessage("RFID Attendance", "Wi-Fi not configured", "Scan your card");
    delay(1500);
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  showMessage("Connecting to Wi-Fi", "Please wait...");
  Serial.print("Connecting to Wi-Fi");

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
    showMessage("Wi-Fi Connected", WiFi.localIP().toString(), "Scan your card");
  } else {
    Serial.println("Wi-Fi connection failed. RFID local mode remains available.");
    showMessage("Wi-Fi unavailable", "Local scan enabled", "Cloud logging off");
  }
  delay(1500);
}

void handleCard(const String& uid) {
  Serial.println();
  Serial.println("RFID UID: " + uid);

  int studentIndex = findStudentByUID(uid);
  if (studentIndex < 0) {
    Serial.println("Unregistered card.");
    showMessage("Invalid Card", "Not Registered", uid);
    beepError();
    delay(1200);
    showMessage("RFID ATTENDANCE", "System Ready", "Scan Your Card");
    return;
  }

  Student& student = students[studentIndex];

  // Toggle between login and logout each time this registered card is scanned.
  student.loggedIn = !student.loggedIn;
  String status = student.loggedIn ? "LOGIN" : "LOGOUT";

  Serial.println("Student: " + String(student.name));
  Serial.println("Status: " + status);

  if (student.loggedIn) {
    showMessage("WELCOME!", student.name, "LOGGING IN...", "Please wait");
  } else {
    showMessage("GOODBYE!", student.name, "LOGGING OUT...", "Please wait");
  }
  beepSuccess();

  bool sent = sendAttendanceToGoogleSheets(uid, String(student.name), status);
  if (sent) {
    showMessage(student.loggedIn ? "LOGIN SUCCESS" : "LOGOUT SUCCESS",
                student.name, "Cloud record saved");
  } else {
    showMessage(student.loggedIn ? "LOGIN SUCCESS" : "LOGOUT SUCCESS",
                student.name, "Local only / cloud N/A");
  }

  delay(1800);
  showMessage("RFID ATTENDANCE", "System Ready", "Scan Your Card");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  // NOTE: If your actual wiring uses different I2C pins, change these.
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lcd.init();
  lcd.backlight();
  showMessage("RFID ATTENDANCE", "Starting system...");

  // Use explicit SPI pins for the ESP32.
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, RFID_SS_PIN);
  rfid.PCD_Init();
  delay(100);

  Serial.println("RFID Attendance System starting...");
  Serial.println("Place an RFID card near the RC522 reader.");

  connectWiFi();
  showMessage("RFID ATTENDANCE", "System Ready", "Scan Your Card");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = readCardUID();

  // Avoid duplicate toggles when a card is held near the reader.
  unsigned long now = millis();
  bool duplicateScan = (uid == lastScannedUID &&
                        now - lastScanMillis < SCAN_COOLDOWN_MS);
  lastScannedUID = uid;
  lastScanMillis = now;

  if (!duplicateScan) {
    handleCard(uid);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
