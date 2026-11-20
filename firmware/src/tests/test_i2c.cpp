// PRUEBA: escaner I2C (nodos 1 y 2)
// Conecta VCC de los sensores al pin "Ve" de la Heltec, GND a GND,
// SDA a GPIO6 y SCL a GPIO7.
#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "hw.h"

const char* nombreDe(uint8_t a) {
  switch (a) {
    case 0x33: return "MLX90640 (termografia)";
    case 0x38: return "AHT20 (temperatura y humedad)";
    case 0x40: case 0x41: case 0x42: case 0x43: return "INA3221 (potencia)";
    case 0x53: return "LTR390 (UV)";
    case 0x76: case 0x77: return "BMP280 (presion)";
    default:   return "desconocido";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  vextOn();
  Wire1.begin(I2C_SDA, I2C_SCL);
  Serial.println("Escaner I2C (SDA=GPIO6, SCL=GPIO7)");
}

void loop() {
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire1.beginTransmission(a);
    if (Wire1.endTransmission() == 0) {
      Serial.printf("  0x%02X  %s\n", a, nombreDe(a));
      n++;
    }
  }
  if (n == 0) Serial.println("  Nada encontrado: revisa alimentacion (Ve), SDA/SCL y GND");
  Serial.println("---");
  delay(3000);
}
