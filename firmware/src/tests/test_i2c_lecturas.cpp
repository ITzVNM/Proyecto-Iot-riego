// PRUEBA: lecturas I2C por nodo
//   -DROL_NODO1 -> AHT20 + BMP280
//   -DROL_NODO2 -> LTR390 + INA3221 (con diagnostico de registros)
#include <Arduino.h>
#include <Wire.h>
#include "pins.h"
#include "hw.h"

#if defined(ROL_NODO1)
  #include <Adafruit_AHTX0.h>
  #include <Adafruit_BMP280.h>
  Adafruit_AHTX0 aht;
  Adafruit_BMP280 bmp(&Wire1);
#elif defined(ROL_NODO2)
  #include <Adafruit_LTR390.h>
  Adafruit_LTR390 ltr;

  #define INA_ADDR   0x40   // cambia si tu placa tiene otra direccion (ver escaner)
  #define SHUNT_OHM  0.1f   // las placas traen R100 = 0,1 ohm

  // INA3221: registros 0x01/0x02 (canal 1), 0x03/0x04 (canal 2), 0x05/0x06 (canal 3)
  // 0xFE = ID del fabricante (0x5449), 0xFF = ID del chip (0x3220)
  static bool ina16(uint8_t reg, int16_t* out) {
    Wire1.beginTransmission(INA_ADDR);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom((int)INA_ADDR, 2) != 2) return false;
    uint8_t h = Wire1.read();
    uint8_t l = Wire1.read();
    *out = (int16_t)((h << 8) | l);
    return true;
  }

  static bool inaCanal(int canal, float* volts, float* mA, int16_t* rawShunt, int16_t* rawBus) {
    uint8_t regShunt = 0x01 + 2 * (canal - 1);
    uint8_t regBus   = 0x02 + 2 * (canal - 1);
    if (!ina16(regShunt, rawShunt)) return false;
    if (!ina16(regBus, rawBus)) return false;
    *mA    = (*rawShunt >> 3) * 0.04f / SHUNT_OHM;   // 40 uV por paso -> mV / ohm = mA
    *volts = (*rawBus >> 3) * 0.008f;                 // 8 mV por paso
    return true;
  }
#else
  #error "Define ROL_NODO1 o ROL_NODO2"
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);
  vextOn();
  Wire1.begin(I2C_SDA, I2C_SCL);

#if defined(ROL_NODO1)
  Serial.println(aht.begin(&Wire1) ? "AHT20 OK" : "AHT20 NO responde");
  bool ok = bmp.begin(0x76) || bmp.begin(0x77);
  Serial.println(ok ? "BMP280 OK" : "BMP280 NO responde");
#else
  if (ltr.begin(&Wire1)) {
    ltr.setMode(LTR390_MODE_UVS);
    ltr.setGain(LTR390_GAIN_18);
    ltr.setResolution(LTR390_RESOLUTION_18BIT);
    Serial.println("LTR390 OK");
  } else {
    Serial.println("LTR390 NO responde");
  }
  int16_t id;
  if (ina16(0xFE, &id)) Serial.printf("INA3221 fabricante=0x%04X (esperado 0x5449)\n", (unsigned)(uint16_t)id);
  if (ina16(0xFF, &id)) Serial.printf("INA3221 chip=0x%04X (esperado 0x3220)\n", (unsigned)(uint16_t)id);
#endif
}

void loop() {
#if defined(ROL_NODO1)
  sensors_event_t hum, temp;
  aht.getEvent(&hum, &temp);
  Serial.printf("AHT20: T=%.2f C  HR=%.1f %% | BMP280: T=%.2f C  P=%.1f hPa\n",
                temp.temperature, hum.relative_humidity,
                bmp.readTemperature(), bmp.readPressure() / 100.0);
#else
  if (ltr.newDataAvailable()) {
    uint32_t uvs = ltr.readUVS();
    // Sensibilidad segun datasheet: 2300 cuentas/UVI a ganancia 18x y 20 bits (400 ms).
    // Con 18 bits (100 ms): 2300 * (100/400) = 575 cuentas por UVI.
    Serial.printf("LTR390: UV crudo=%lu  UVI~%.2f\n", (unsigned long)uvs, uvs / 575.0);
  }
  for (int c = 1; c <= 3; c++) {
    float v, mA;
    int16_t rs, rb;
    if (inaCanal(c, &v, &mA, &rs, &rb)) {
      Serial.printf("INA3221 CH%d: %.3f V  %.1f mA  %.1f mW | reg 0x%02X=0x%04X (shunt %ld uV)  reg 0x%02X=0x%04X\n",
                    c, v, mA, v * mA,
                    0x01 + 2 * (c - 1), (unsigned)(uint16_t)rs, (long)((rs >> 3) * 40),
                    0x02 + 2 * (c - 1), (unsigned)(uint16_t)rb);
    } else {
      Serial.printf("INA3221 CH%d: sin respuesta (direccion 0x%02X?)\n", c, INA_ADDR);
    }
  }
#endif
  Serial.println("---");
  delay(2000);
}
