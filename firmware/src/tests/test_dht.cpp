// PRUEBA: DHT22 (nodo 3). Datos en GPIO6; VCC al pin Ve; resistencia de 10k entre datos y VCC
// si tu DHT22 es el de 4 patas (el modulo de 3 patas ya la trae).
#include <Arduino.h>
#include <DHT.h>
#include "pins.h"
#include "hw.h"

DHT dht(PIN_DHT, DHT22);

void setup() {
  Serial.begin(115200);
  delay(1000);
  vextOn(1000);   // el DHT22 necesita un segundo para estabilizarse
  dht.begin();
  Serial.println("Prueba DHT22");
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) Serial.println("DHT22: error de lectura (revisa cableado y resistencia)");
  else Serial.printf("DHT22: T=%.1f C  HR=%.1f %%\n", t, h);
  delay(2500);   // el DHT22 admite una lectura cada 2 s como minimo
}
