#pragma once
#include <Arduino.h>
#include "pins.h"

// Vext = salida de 3,3 V para sensores externos (pin "Ve" del header).
// Con LOW queda encendida. Conecta la alimentacion de los sensores a ese pin.
inline void vextOn(uint32_t esperaMs = 100) {
  pinMode(PIN_VEXT, OUTPUT);
  digitalWrite(PIN_VEXT, LOW);
  delay(esperaMs);   // tiempo para que los sensores arranquen
}

inline void vextOff() {
  digitalWrite(PIN_VEXT, HIGH);
}
