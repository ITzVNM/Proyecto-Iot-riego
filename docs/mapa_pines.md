# Mapa de pines (Heltec WiFi LoRa 32 V3)

Definido en `firmware/include/pins.h`.

| Función | Pines |
|---|---|
| RS485, nodos 2 y 3 (módulo TTL-RS485) | RX en GPIO5, TX en GPIO4 |
| I2C, nodos 1 y 2 (se usa `Wire1`) | SDA en GPIO6, SCL en GPIO7 |
| SPI de los 3 MAX31865, nodo 1 | SCK 39, MISO 40, MOSI 41; selección 42, 38 y 21 |
| DHT22, nodo 3 | GPIO6 |
| Relé de la electroválvula, nodo 3 | GPIO7 (activo en bajo por defecto) |
| Caudalímetro YF-S201, nodo 3 | GPIO2 (interrupción) |
| Alimentación de sensores | Vext, GPIO36 (nivel bajo = encendido) |
| Habilitación de 12 V del piranómetro | GPIO48 (reservado para la etapa de energía) |
| Lectura de batería | GPIO1 con habilitación en GPIO37 (reservado) |

Ya ocupados por la placa: 8 a 14 (LoRa), 17 y 18 (OLED), 0 (botón PRG), 35 (LED), 19 y 20 (USB).

El GPIO21 es también el pin de reinicio del OLED; se reutiliza como selección de la tercera placa MAX31865 porque el OLED no se usa en los nodos.

## Sensores por nodo

| Nodo | Elemento | Interfaz y dirección |
|---|---|---|
| 1 | 3 × PT100 con MAX31865 | SPI, un CS por placa |
| 1 | AHT20 y BMP280 | I2C 0x38 y 0x76/0x77 |
| 2 | Piranómetro PR-300AL-RA-N01 | RS485 Modbus, 4800 baudios, dirección 1 |
| 2 | LTR390 e INA3221 (canal 1) | I2C 0x53 y 0x40 |
| 3 | Suelo multiparámetro | RS485 Modbus, 9600 baudios, dirección 2 |
| 3 | DHT22, YF-S201, relé y electroválvula 12 V | 1 hilo, pulsos, salida digital |
