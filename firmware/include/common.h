#pragma once
#include <Arduino.h>

#define FREQ_MHZ 915.0

// Tipos de mensaje
enum : uint8_t { MSG_POLL = 1, MSG_TERMICO = 2, MSG_RADIACION = 3, MSG_CULTIVO = 4 };

// Ordenes para la electrovalvula (solo las atiende el nodo 3)
enum : uint8_t { CMD_NADA = 0, CMD_ABRIR = 1, CMD_CERRAR = 2 };

#define NODO_TERMICO    1
#define NODO_RADIACION  2
#define NODO_CULTIVO    3

// Gateway -> nodo (8 bytes)
struct __attribute__((packed)) TramaPoll {
  uint8_t  tipo;      // MSG_POLL
  uint8_t  destino;   // ID del nodo consultado
  uint16_t seq;
  uint8_t  cmd;       // CMD_*
  uint8_t  cmd_id;    // identificador de la orden (el nodo ignora repetidas)
  uint16_t dur_s;     // segundos de riego al abrir (0 = valor por defecto del nodo)
};

// Nodo 1 (termico) -> gateway
// estado: bit0 AHT20, bit1 BMP280, bit2 PT100 #1, bit3 PT100 #2, bit4 PT100 #3
struct __attribute__((packed)) TramaTermico {
  uint8_t  tipo;          // MSG_TERMICO
  uint8_t  origen;
  uint16_t seq;
  uint8_t  estado;
  int16_t  pt_x100[3];    // PT100, °C * 100
  int16_t  tamb_x100;     // temperatura ambiente (AHT20), °C * 100
  uint16_t hr_x100;       // humedad relativa, % * 100
  uint16_t pres_x10;      // presion (BMP280), hPa * 10
};

// Nodo 2 (radiacion y potencia) -> gateway
// estado: bit0 piranometro, bit1 LTR390, bit2 INA3221
struct __attribute__((packed)) TramaRadiacion {
  uint8_t  tipo;          // MSG_RADIACION
  uint8_t  origen;
  uint16_t seq;
  uint8_t  estado;
  uint16_t irr_wm2;       // irradiancia, W/m2
  uint16_t uvi_x100;      // indice UV * 100
  uint16_t vpanel_mV;     // voltaje del panel, mV
  int16_t  ipanel_mA;     // corriente del panel, mA
};

// Nodo 3 (cultivo) -> gateway
// estado: bit0 suelo, bit1 DHT22
struct __attribute__((packed)) TramaCultivo {
  uint8_t  tipo;          // MSG_CULTIVO
  uint8_t  origen;
  uint16_t seq;
  uint8_t  estado;
  int16_t  suelo_temp_x10;   // °C * 10 (con signo)
  uint16_t suelo_hum_x10;    // % * 10
  uint16_t ec;               // uS/cm
  uint16_t sal;
  uint16_t n;                // mg/kg
  uint16_t p;                // mg/kg
  uint16_t k;                // mg/kg
  uint16_t ph_x10;           // pH * 10
  int16_t  dht_temp_x10;     // °C * 10
  uint16_t dht_hr_x10;       // % * 10
  uint16_t caudal_x100;      // L/min * 100 (promedio desde la consulta anterior)
  uint32_t volumen_ml;       // agua acumulada desde el arranque, mL
  uint8_t  valvula;          // 0 cerrada, 1 abierta
  uint8_t  ack_id;           // ultima orden recibida
};

static_assert(sizeof(TramaPoll) <= 64 && sizeof(TramaTermico) <= 64 &&
              sizeof(TramaRadiacion) <= 64 && sizeof(TramaCultivo) <= 64,
              "las tramas deben caber en el buffer de 64 bytes");
