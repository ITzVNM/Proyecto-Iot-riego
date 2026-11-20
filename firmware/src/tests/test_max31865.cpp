// PRUEBA: tres PT100 con tres MAX31865 (nodo 1)
// Los tres modulos comparten SCK/MISO/MOSI y cada uno tiene su CS.
#include <Arduino.h>
#include <Adafruit_MAX31865.h>
#include "pins.h"
#include "hw.h"

#define RNOMINAL 100.0   // PT100
#define RREF     430.0   // resistencia de referencia de la placa PT100 (4300 si fuera PT1000)
#define HILOS    MAX31865_3WIRE   // MAX31865_2WIRE / _3WIRE / _4WIRE segun tu sonda y puentes

Adafruit_MAX31865 pt1(PT_CS1, PT_MOSI, PT_MISO, PT_SCK);
Adafruit_MAX31865 pt2(PT_CS2, PT_MOSI, PT_MISO, PT_SCK);
Adafruit_MAX31865 pt3(PT_CS3, PT_MOSI, PT_MISO, PT_SCK);

void leer(const char* nombre, Adafruit_MAX31865& p) {
  uint16_t rtd = p.readRTD();
  float temp = p.temperature(RNOMINAL, RREF);
  uint8_t fallo = p.readFault();
  if (fallo) {
    Serial.printf("%s: FALLO 0x%02X ->", nombre, fallo);
    if (fallo & MAX31865_FAULT_HIGHTHRESH) Serial.print(" RTD>umbral alto");
    if (fallo & MAX31865_FAULT_LOWTHRESH)  Serial.print(" RTD<umbral bajo");
    if (fallo & MAX31865_FAULT_REFINLOW)   Serial.print(" REFIN- > 0.85 Vbias");
    if (fallo & MAX31865_FAULT_REFINHIGH)  Serial.print(" REFIN- < 0.85 Vbias");
    if (fallo & MAX31865_FAULT_RTDINLOW)   Serial.print(" RTDIN- < 0.85 Vbias");
    if (fallo & MAX31865_FAULT_OVUV)       Serial.print(" sobre/sub tension");
    Serial.println();
    p.clearFault();
  } else {
    Serial.printf("%s: RTD=%u  R=%.2f ohm  T=%.2f C\n",
                  nombre, rtd, rtd / 32768.0 * RREF, temp);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  vextOn();
  pt1.begin(HILOS);
  pt2.begin(HILOS);
  pt3.begin(HILOS);
  Serial.println("Prueba de 3 x PT100 (MAX31865)");
}

void loop() {
  leer("PT100 #1", pt1);
  leer("PT100 #2", pt2);
  leer("PT100 #3", pt3);
  Serial.println("---");
  delay(2000);
}
