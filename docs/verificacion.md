# Guía de verificación

Resultado esperado de cada entorno de prueba de PlatformIO (una placa conectada a la vez).

| Entorno | Qué verifica | Salida esperada |
|---|---|---|
| `t_i2c_scan` | Dispositivos I2C | 0x38 (AHT20), 0x76 o 0x77 (BMP280), 0x53 (LTR390), 0x40 (INA3221) |
| `t_i2c_nodo1` | AHT20 y BMP280 | Temperatura, humedad y presión coherentes con el ambiente |
| `t_i2c_nodo2` | LTR390 e INA3221 | UV crudo; voltaje y corriente del canal 1 |
| `t_pt100` | MAX31865 | Resistencia cercana a 110 Ω y temperatura ambiente por sonda; `FALLO` si una placa no tiene sonda |
| `t_rs485_nodo2` | Piranómetro | Respuesta a 4800 baudios, dirección 1; irradiancia en W/m² |
| `t_rs485_nodo3` | Suelo | Respuesta a 9600 baudios, dirección 2; 8 parámetros |
| `t_dht` | DHT22 | Temperatura y humedad del aire |
| `t_flujo` | YF-S201 | Pulsos por segundo y caudal al pasar agua |
| `t_rele` | Relé | Conmuta 3 s encendido y 5 s apagado (probar sin la válvula) |

## Sistema integrado

1. Monitor serie del gateway: una línea JSON por nodo cada unos 10 s con RSSI y SNR.
2. Dashboard: variables de los tres nodos y estado del enlace en línea.
3. Orden de riego: en el monitor del gateway, la tecla `a` abre la válvula 10 s sin pasar por MQTT; desde el dashboard, el botón "Abrir 30 s" muestra avisos de envío, recepción y confirmación.
4. Archivos: aparecen los CSV y el respaldo JSON en la carpeta de datos.
5. Asesor de IA: `GET /api/resumen24h?demo=1` devuelve el resumen de ejemplo; al ejecutar a mano el flujo de n8n, el diagnóstico aparece en el dashboard.
