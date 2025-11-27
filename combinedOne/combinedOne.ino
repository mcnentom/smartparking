#include <SPI.h>
#include <MFRC522.h>
#include <MD_MAX72xx.h>

// --------------------------------------------------
// DOT MATRIX CONFIG (Software SPI as you wired it)
// --------------------------------------------------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define MATRIX_DIN   0    // GPIO0
#define MATRIX_CS    5    // GPIO5
#define MATRIX_CLK   15   // GPIO15

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, MATRIX_DIN, MATRIX_CLK, MATRIX_CS, MAX_DEVICES);

// 8x8 symbols
// --- SYMBOLS (8x8) ---
const uint8_t symbol_check[8] = {
  0x00, 0x01, 0x03, 0x86, 0xCC, 0x78, 0x30, 0x00
};

const uint8_t symbol_x[8] = {
  0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};


// --------------------------------------------------
// ULTRASONIC CONFIG
// --------------------------------------------------
#define TRIG_PIN 0   // GPIO0 (D3)
#define ECHO_PIN 4   // GPIO4 (D2)


// --------------------------------------------------
// RC522 CONFIG (Hardware SPI)
// --------------------------------------------------
#define RST_PIN  16   // D0
#define SS_PIN   2    // D4

MFRC522 mfrc522(SS_PIN, RST_PIN);

// Authorized RFID card
const byte authorized1[4] = {0xCC, 0xD2, 0x05, 0x02};


// --------------------------------------------------
// Draw Symbol On Dot Matrix
// --------------------------------------------------
void drawSymbol(const uint8_t symbol[8]) {
  for (uint8_t col = 0; col < 8; col++) {
    mx.setColumn(0, col, symbol[col]);
  }
}


// --------------------------------------------------
// Read Ultrasonic Sensor
// --------------------------------------------------
long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  long distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}


// --------------------------------------------------
// Check Authorization
// --------------------------------------------------
bool checkAuthorization(byte *uid) {
  for (int i = 0; i < 4; i++) {
    if (uid[i] != authorized1[i]) return false;
  }
  return true;
}


// --------------------------------------------------
// RFID Scan
// --------------------------------------------------
void processRFID() {
  Serial.println("Waiting for RFID card...");

  unsigned long startTime = millis();

  while (millis() - startTime < 5000) {

    if (mfrc522.PICC_IsNewCardPresent() &&
        mfrc522.PICC_ReadCardSerial()) {

      // Print UID
      Serial.print("Card UID: ");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Check authorization
      bool authorized = checkAuthorization(mfrc522.uid.uidByte);

      mx.clear();
      if (authorized) {
        Serial.println("ACCESS GRANTED ✔");
        drawSymbol(symbol_check);
      } else {
        Serial.println("ACCESS DENIED ❌");
        drawSymbol(symbol_x);
      }

      delay(1500);
      mx.clear();

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return;
    }
  }

  Serial.println("RFID TIMEOUT - No card detected.");
}


// --------------------------------------------------
// SETUP
// --------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Start MATRIX (software SPI on your pins)
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);
  mx.clear();

  // Start RC522 (hardware SPI)
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("System Ready.");
}


// --------------------------------------------------
// MAIN LOOP
// --------------------------------------------------
void loop() {
  long distance = readUltrasonic();

  if (distance <= 30 && distance > 0) {
    Serial.println("Object detected → Trigger RFID scan");
    processRFID();
    delay(4000);
  }

  delay(3000);
}
