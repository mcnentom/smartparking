#include <SPI.h>
#include <MFRC522.h>
#include <MD_MAX72xx.h>

// -------- LED MATRIX CONFIG -----------
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define DATA_PIN  5   // DIN
#define CLK_PIN   4   // CLK
#define CS_PIN    15   // CS (LOAD)

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// --- SYMBOLS (8x8) ---
const uint8_t symbol_check[8] = {
  0x00, 0x01, 0x03, 0x86, 0xCC, 0x78, 0x30, 0x00
};

const uint8_t symbol_x[8] = {
  0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

// ---------- RC522 CONFIG ------------
#define RST_PIN  16       // Reset
#define SS_PIN   2      // SDA

MFRC522 mfrc522(SS_PIN, RST_PIN);

// ------ AUTHORIZED CARDS ------
const byte authorized1[4] = {0xCC, 0xD2, 0x05, 0x02};
// const byte authorized2[4] = {0xB1, 0x2A, 0x06, 0x02};

// --------- Helper: Draw 8x8 symbol ----------
void drawSymbol(uint8_t module, const uint8_t* symbol) {
  for (uint8_t col = 0; col < 8; col++) {
    mx.setColumn(module, col, symbol[col]);
  }
}

// Check if UID matches an authorized card
bool checkAuthorization(byte *uid) {
  bool match1 = true;
  // bool match2 = true;

  for (int i = 0; i < 4; i++) {
    if (uid[i] != authorized1[i]) match1 = false;
    // if (uid[i] != authorized2[i]) match2 = false;
  }

  return match1;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  // LED Matrix init
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);
  mx.clear();

  // RC522 init
  SPI.begin();
  mfrc522.PCD_Init();

  Serial.println("Place card on RC522...");
}

void loop() {

  // No new card → do nothing
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  bool authorized = checkAuthorization(mfrc522.uid.uidByte);

  // Clear display before showing a symbol
  mx.clear();

  if (authorized) {
    Serial.println("ACCESS GRANTED ✔");
    drawSymbol(0, symbol_check);   // show tick on module 0
  } else {
    Serial.println("ACCESS DENIED ❌");
    drawSymbol(0, symbol_x);       // show X on module 0
  }

  delay(2000);
  mx.clear();

  // Stop crypto to allow next card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
