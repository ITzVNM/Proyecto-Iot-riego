// PRUEBA: rele de la electrovalvula (nodo 3), señal en GPIO7
// Pruebala PRIMERO sin la valvula conectada: se debe oir el clic o ver el LED del modulo.
#include <Arduino.h>
#include "pins.h"

#define RELE_ACTIVO_BAJO 1      // 1 si el modulo se activa con LOW (lo mas comun), 0 si con HIGH
#define PERMITIR_ACTIVAR 1      // 0 = nunca activa, solo deja el rele apagado
#define MAX_ON_MS        3000   // limite de seguridad: nunca queda activo mas de esto

#if RELE_ACTIVO_BAJO
  #define NIVEL_ON  LOW
  #define NIVEL_OFF HIGH
#else
  #define NIVEL_ON  HIGH
  #define NIVEL_OFF LOW
#endif

void releOff() { digitalWrite(PIN_RELE, NIVEL_OFF); }
void releOn()  { digitalWrite(PIN_RELE, NIVEL_ON); }

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RELE, OUTPUT);
  releOff();   // siempre arranca apagado
  delay(1000);
  Serial.println("Prueba de rele (maximo 3 s encendido por ciclo)");
}

void loop() {
#if PERMITIR_ACTIVAR
  Serial.println("Rele ON");
  releOn();
  delay(MAX_ON_MS);
  releOff();
  Serial.println("Rele OFF");
#endif
  delay(5000);
}
