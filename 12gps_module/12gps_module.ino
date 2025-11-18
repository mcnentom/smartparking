#include <SoftwareSerial.h>
SoftwareSerial gpsSerial(D7, D8);

void setup() {
  Serial.begin(115200);
  while(!Serial);
  gpsSerial.begin(9600);
  Serial.println("Reading raw GPS data...");
}

void loop() {
  // delay(3000);
  if (gpsSerial.available()) {
    
    Serial.write(gpsSerial.read());
  }
}