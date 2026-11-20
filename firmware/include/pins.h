#pragma once
// ============================================================
//  Mapa de pines definitivo (Heltec WiFi LoRa 32 V3)
//  Ya ocupados por la placa: 8-14 (LoRa), 17-18 (OLED),
//  0 (PRG), 35 (LED), 43-44 (USB-serie), 19-20 (USB)
//  Evitar: 3, 45, 46 (arranque) y 26 (memoria)
// ============================================================

// ---- Placa ----
#define PIN_VEXT      36   // LOW = enciende el 3,3 V externo (pin "Ve" del header)
#define PIN_BAT_ADC    1   // lectura de bateria (divisor 100k/390k)
#define PIN_ADC_CTRL  37   // habilita el divisor (el nivel depende de la version V3 / V3.2)

// ---- RS485 (nodos 2 y 3) ----
#define RS485_RX       5   // RX del ESP32  <- TXD del modulo TTL-RS485 (cableado probado)
#define RS485_TX       4   // TX del ESP32  -> RXD del modulo TTL-RS485 (cableado probado)

// ---- I2C (nodos 1 y 2), se usa Wire1 para no chocar con el OLED ----
#define I2C_SDA        6
#define I2C_SCL        7

// ---- SPI de los 3 MAX31865 (nodo 1) ----
#define PT_SCK        39
#define PT_MISO       40
#define PT_MOSI       41
#define PT_CS1        42
#define PT_CS2        38
#define PT_CS3        21   // reutiliza el pin del reset del OLED (el OLED no se usa en los nodos)

// ---- Nodo 2 ----
#define PIN_12V_EN    48   // habilita los 12 V del piranometro (MOSFET)

// ---- Nodo 3 (no usa I2C, por eso puede reutilizar 6 y 7) ----
#define PIN_DHT        6   // datos del DHT22
#define PIN_RELE       7   // activa el rele de la electrovalvula
#define PIN_FLUJO      2   // pulsos del YF-S201
