#include <MD_MAX72xx.h>
#include <SPI.h>

// Choose your module type
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// Number of 8x8 matrices chained
#define MAX_DEVICES 4  // adjust to how many modules you have
// Pin setup for NodeMCU (ESP8266)
#define DATA_PIN  0   // DIN
#define CLK_PIN   15   // CLK
#define CS_PIN    5  // CS (LOAD)


// Create the LED matrix object
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
// --- Some symbols (8x8 bitmaps) ---
const uint8_t symbol_check[8] = {
  0x00, 0x01, 0x03, 0x86, 0xCC, 0x78, 0x30, 0x00
};

const uint8_t symbol_x[8] = {
  0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81
};

const uint8_t symbol_car[8] = {
  0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x18, 0x00
};

// Helper function: draw a symbol on one 8x8 matrix
void drawSymbol(uint8_t module, const uint8_t* symbol) {
  for (uint8_t col = 0; col < 8; col++) {
    mx.setColumn(module, col, symbol[col]);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("MAX7219 Test Starting...");
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5);  // brightness 0–15
  mx.clear();
}

void loop() {
  // Cycle through modules and symbols
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    Serial.printf("Showing symbols on module %d\n", i);
    // Show checkmark
    mx.clear();
    drawSymbol(i, symbol_check);
    delay(1000);
    // Show X
    mx.clear();
    drawSymbol(i, symbol_x);
    delay(1000);
    // Show car
    mx.clear();
    drawSymbol(i, symbol_car);
    delay(1000);
  }
  // Show all modules at once
  Serial.println("Lighting all modules with cars...");
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    drawSymbol(i, symbol_car);
  }
  delay(2000);
  mx.clear();
  delay(1000);
}