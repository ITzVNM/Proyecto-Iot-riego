// PRUEBA: sensor de flujo YF-S201 (nodo 3), pulsos en GPIO2
// ATENCION: los pulsos salen al nivel con que alimentas el sensor.
// Con 5 V o mas, usa un divisor (10k arriba / 20k abajo) antes del ESP32.
#include <Arduino.h>
#include "pins.h"

#define PULSOS_POR_LITRO 450.0   // nominal; calibra con un volumen conocido
#define FLUJO_PULLUP     0       // 1 solo si la salida es colector abierto y NO usas divisor

volatile uint32_t pulsos = 0;
void IRAM_ATTR contar() { pulsos++; }

uint32_t totalPulsos = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
#if FLUJO_PULLUP
  pinMode(PIN_FLUJO, INPUT_PULLUP);
#else
  pinMode(PIN_FLUJO, INPUT);
#endif
  attachInterrupt(digitalPinToInterrupt(PIN_FLUJO), contar, RISING);
  Serial.println("Prueba de flujo: haz pasar agua por el sensor (1 a 30 L/min)");
}

void loop() {
  delay(1000);
  noInterrupts();
  uint32_t p = pulsos;
  pulsos = 0;
  interrupts();

  totalPulsos += p;
  float lMin  = (p / PULSOS_POR_LITRO) * 60.0f;   // pulsos en 1 s -> L/min
  float total = totalPulsos / PULSOS_POR_LITRO;
  Serial.printf("pulsos/s=%lu  caudal=%.2f L/min  total=%.3f L  (pulsos acumulados=%lu)\n",
                (unsigned long)p, lMin, total, (unsigned long)totalPulsos);
}
