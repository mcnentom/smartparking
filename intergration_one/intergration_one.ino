#include <SPI.h>
#include <MFRC522.h>
#include <MD_MAX72xx.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// ---------------------- WIFI ----------------------
const char* WIFI_SSID = "Redmi";
const char* WIFI_PASS = "mcnenangel";

const String BASE_URL = "http://10.229.139.219:4000";   // YOUR SERVER IP
String slotId = "lot-2-S04";                               // Parking slot ID

// ---------------------- DOT MATRIX ------------------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define MATRIX_DIN   0 
#define MATRIX_CS    5 
#define MATRIX_CLK   15

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MATRIX_DIN, MATRIX_CLK, MATRIX_CS, MAX_DEVICES);

// Symbols
const uint8_t symbol_check[8] = {
  0x00,0x01,0x03,0x86,0xCC,0x78,0x30,0x00
};

const uint8_t symbol_x[8] = {
  0x81,0x42,0x24,0x18,0x18,0x24,0x42,0x81
};

// ---------------------- ULTRASONIC ------------------
#define TRIG_PIN 0
#define ECHO_PIN 4

// ---------------------- RFID ------------------------
#define RST_PIN  16
#define SS_PIN   2
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Authorized UID
const byte authorized1[4] = {0xCC, 0xD2, 0x05, 0x02};

// ---------------------- DRAW MATRIX -----------------
void drawSymbol(const uint8_t symbol[8]) {
  for (uint8_t col = 0; col < 8; col++) {
    mx.setColumn(0, col, symbol[col]);
  }
}

// ---------------------- READ ULTRASONIC -------------
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  long distance = duration * 0.034 / 2;
  return distance;
}

// ---------------------- API UPDATE ------------------
String updateSlotToServer(String rfid) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return "fail";
  }

  WiFiClient client;
  HTTPClient http;

  String url = BASE_URL + "/api/auth/rfid";

  Serial.println("POST → " + url);

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin FAILED!");
    return "fail";
  }

  http.addHeader("Content-Type", "application/json");

  String body =
    "{ \"rfid\": \"" + rfid + "\", \"slotId\": \"" + slotId + "\" }";

  Serial.println("Body: " + body);

  int code = http.POST(body);

  if (code > 0) {
    Serial.print("Response: ");
    Serial.println(code);
    String resp = http.getString();
    Serial.println(resp);

    http.end();
    return resp;    // <── return server message: "success" or "fail"
  } else {
    Serial.print("POST FAILED → ");
    Serial.println(code);
  }

  http.end();
  return "fail";   // default if no proper response
}


// ---------------------- RFID AUTH -------------------
bool checkAuthorization(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != authorized1[i]) return false;
  }
  return true;
}

String extractStatus(String json) {
  int s = json.indexOf("\"status\"");
  if (s < 0) return "UNKNOWN";

  int colon = json.indexOf(":", s);
  int quote1 = json.indexOf("\"", colon + 1);
  int quote2 = json.indexOf("\"", quote1 + 1);

  return json.substring(quote1 + 1, quote2);
}

String updateOccupiedStatus(bool occupied) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return "fail";
  }

  WiFiClient client;
  HTTPClient http;

  String url = BASE_URL + "/api/slot/" + slotId + "/update";
  Serial.println("POST → " + url);

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin FAILED!");
    return "fail";
  }

  http.addHeader("Content-Type", "application/json");

  String body = String("{ \"occupied\": ") + (occupied ? "true" : "false") + " }";

  Serial.println("Body: " + body);

  int code = http.POST(body);

  if (code > 0) {
    Serial.print("Response: ");
    Serial.println(code);
    String resp = http.getString();
    Serial.println(resp);
    http.end();
    return resp;
  }

  http.end();
  return "fail";
}

// ---------------------- RFID PROCESS ----------------
void processRFID() {
  Serial.println("Waiting for RFID...");

  unsigned long start = millis();

  while (millis() - start < 5000) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

      String uid = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        uid += String(mfrc522.uid.uidByte[i], HEX);
      }

      // Send RFID + slotId
      String result = updateSlotToServer(uid);

      mx.clear();

      String status = extractStatus(result);

      Serial.print("Server Status: ");
      Serial.println(status);

     if (status == "SUCCESS") {
      drawSymbol(symbol_check);   // ✓
      Serial.println("Authenticated. Waiting 10 seconds to check occupancy...");
      
      delay(15000);  // WAIT 10 seconds

      long d = readUltrasonic();
      bool isOccupied = (d > 0 && d <= 10);

      Serial.print("Distance after 10 seconds: ");
      Serial.print(d);
      Serial.println(" cm");

      Serial.print("Sending occupancy update: ");
      Serial.println(isOccupied ? "true" : "false");

      updateOccupiedStatus(isOccupied);
      
    } else {
      drawSymbol(symbol_x);       // X
    }


      delay(2000);
      mx.clear();

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }
  }
}


void blinkWaitingX() {
  unsigned long blinkStart = millis();

  while (true) {
    // Check if car is STILL present
    long d = readUltrasonic();
    if (d <= 0 || d > 30) {
      mx.clear();
      return;   // Car left → stop blinking
    }

    // Check RFID
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      mx.clear();
      return;   // Go back to normal → processRFID() will run next
    }

    // Blink ON
    drawSymbol(symbol_x);
    delay(300);

    // Blink OFF
    mx.clear();
    delay(300);
  }
}


// ---------------------- SETUP -----------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);
  mx.clear();

  SPI.begin();
  mfrc522.PCD_Init();

  // WIFI CONNECT
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Serial.println("System Ready");
}

// ---------------------- LOOP ------------------------
bool lastOccupied = false;
unsigned long lastUpdateTime = 0;

void loop() {
  long distance = readUltrasonic();
  bool occupied = (distance > 0 && distance <= 30);

  // --- CAR ARRIVES ---
  if (occupied && !lastOccupied) {
    Serial.println("Car Detected");

    blinkWaitingX();
    processRFID();

    lastOccupied = true;
  }

  // --- CAR LEAVES ---
  if (!occupied && lastOccupied) {
    Serial.println("Car Left");

    // POST occupied false when car leaves
    Serial.println("Sending occupied=false (car left)");
    updateOccupiedStatus(false);

    lastOccupied = false;
  }

  // --- PERIODIC UPDATE EVERY 15 SECONDS ---
  if (millis() - lastUpdateTime >= 15000) {
    lastUpdateTime = millis();

    long d = readUltrasonic();
    bool occ = (d > 0 && d <= 10);

    Serial.print("15s periodic check → occupied = ");
    Serial.println(occ ? "true" : "false");

    updateOccupiedStatus(occ);
  }

  delay(200);
}
