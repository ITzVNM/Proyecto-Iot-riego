#pragma once
#include <Arduino.h>
#include "pins.h"

inline uint16_t crc16(const uint8_t* d, size_t n) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; i++) {
    crc ^= d[i];
    for (int b = 0; b < 8; b++) crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

inline void abrirRS485(uint32_t baudios) {
  Serial1.end();
  Serial1.begin(baudios, SERIAL_8N1, RS485_RX, RS485_TX);
  delay(50);
}

// Funcion 0x03. Devuelve 0 si todo bien:
//  -1 sin respuesta | -2 CRC malo | -3 longitud rara | -4 excepcion Modbus | -5 otra direccion
inline int leerRegistros(uint8_t addr, uint16_t inicio, uint16_t cuantos, uint16_t* out,
                         uint32_t timeoutMs = 300) {
  uint8_t req[8] = { addr, 0x03, (uint8_t)(inicio >> 8), (uint8_t)inicio,
                     (uint8_t)(cuantos >> 8), (uint8_t)cuantos, 0, 0 };
  uint16_t c = crc16(req, 6);
  req[6] = c & 0xFF;
  req[7] = c >> 8;

  while (Serial1.available()) Serial1.read();
  delay(10);                         // silencio previo (el manual pide >= 4 bytes)
  Serial1.write(req, 8);
  Serial1.flush();

  const size_t esperado = 5 + cuantos * 2;
  uint8_t resp[64];
  size_t n = 0;
  uint32_t t0 = millis();
  while (n < esperado && n < sizeof(resp) && millis() - t0 < timeoutMs) {
    if (Serial1.available()) resp[n++] = Serial1.read();
    if (n == 5 && (resp[1] & 0x80)) break;   // excepcion: solo 5 bytes
  }

  if (n < 5) return -1;
  if (crc16(resp, n - 2) != (uint16_t)(resp[n - 2] | (resp[n - 1] << 8))) return -2;
  if (resp[1] & 0x80) return -4;
  if (n != esperado || resp[2] != cuantos * 2) return -3;
  if (resp[0] != addr) return -5;

  for (uint16_t i = 0; i < cuantos; i++) out[i] = (resp[3 + 2 * i] << 8) | resp[4 + 2 * i];
  return 0;
}
