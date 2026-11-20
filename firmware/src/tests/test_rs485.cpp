// PRUEBA: RS485 / Modbus RTU
//   -DROL_NODO2 -> piranometro (4800, dir. 1)
//   -DROL_NODO3 -> sensor de suelo (9600, dir. 2)
// Al arrancar escanea velocidades y direcciones; despues lee el sensor en bucle.
#include <Arduino.h>
#include "pins.h"
#include "hw.h"
#include "modbus_rtu.h"

#if defined(ROL_NODO2)
  #define NOMBRE "Piranometro"
  #define BAUD   4800
  #define ADDR   1
#elif defined(ROL_NODO3)
  #define NOMBRE "Suelo"
  #define BAUD   9600
  #define ADDR   2
#else
  #error "Define ROL_NODO2 o ROL_NODO3"
#endif

#define ESCANEO_INICIAL 1

void escanear() {
  const uint32_t bauds[] = {4800, 9600, 2400, 19200, 38400, 1200};
  Serial.println("== Escaneo: direcciones 1 a 16 en cada velocidad ==");
  bool alguno = false;
  for (uint32_t bd : bauds) {
    abrirRS485(bd);
    for (int a = 1; a <= 16; a++) {
      uint16_t v = 0;
      int e = leerRegistros(a, 0x0000, 1, &v, 150);
      if (e == 0 || e == -4) {
        Serial.printf("RESPONDE | baudios=%lu | dir=%d | reg0=%u%s\n",
                      (unsigned long)bd, a, v, e == -4 ? " (excepcion Modbus)" : "");
        alguno = true;
      }
    }
  }
  if (!alguno) Serial.println("Sin respuesta: revisa alimentacion, GND comun, A/B (prueba invertirlos) y pines RX/TX");
  Serial.println("== Fin del escaneo ==");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  vextOn();   // alimenta el modulo TTL-RS485 si lo conectaste al pin Ve
  Serial.println("Prueba RS485: " NOMBRE);
#if ESCANEO_INICIAL
  escanear();
#endif
  abrirRS485(BAUD);
}

void loop() {
#if defined(ROL_NODO2)
  uint16_t r = 0;
  int e = leerRegistros(ADDR, 0x0000, 1, &r);
  if (e) Serial.printf("Piranometro: error %d\n", e);
  else   Serial.printf("Piranometro | Irradiancia=%u W/m2\n", r);
#else
  uint16_t r[8];
  int e = leerRegistros(ADDR, 0x0000, 8, r);
  if (e) {
    Serial.printf("Suelo: error %d\n", e);
  } else {
    Serial.printf("Suelo | T=%.1f C | Hum=%.1f %% | EC=%u uS/cm | Sal=%u | N=%u P=%u K=%u mg/kg | pH=%.1f\n",
                  (int16_t)r[0] / 10.0f, r[1] / 10.0f, r[2], r[3], r[4], r[5], r[6], r[7] / 10.0f);
  }
#endif
  delay(2000);
}
