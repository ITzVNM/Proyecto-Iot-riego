# Formato de tramas LoRa

Definidas en `firmware/include/common.h`. Enteros escalados, estructuras empaquetadas. Enlace: 915 MHz, SF9, 125 kHz, CR 4/7, 10 dBm.

| Trama | Bytes | Contenido |
|---|---|---|
| Consulta (gateway a nodo) | 8 | Tipo, destino, secuencia, orden de válvula, identificador de orden, duración |
| Térmica (nodo 1) | 17 | Estado por sensor, 3 PT100 (°C×100), temperatura (×100), humedad (×100), presión (hPa×10) |
| Radiación (nodo 2) | 13 | Estado, irradiancia (W/m²), UVI (×100), voltaje (mV), corriente (mA) |
| Cultivo (nodo 3) | 33 | Estado, 8 variables del suelo, DHT22, caudal (L/min×100), volumen (mL), válvula, confirmación |

Salvaguardas de la válvula en el nodo 3: apertura máxima de 600 s, órdenes con identificador (no se repiten) y cierre si pasan 90 s sin consulta del gateway.
