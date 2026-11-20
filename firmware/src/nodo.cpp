// Firmware de los nodos de campo. El rol lo decide NODE_ID (ver platformio.ini):
//   1 = termico   (3 x PT100 + AHT20/BMP280)
//   2 = radiacion (piranometro + LTR390 + INA3221)
//   3 = cultivo   (suelo + DHT22 + flujo + electrovalvula)
#include <Arduino.h>
#include <RadioLib.h>
#include <math.h>
#include "pins.h"
#include "hw.h"
#include "common.h"
#include "modbus_rtu.h"

SX1262 radio = new Module(8, 14, 12, 13);
volatile bool rxFlag = false;
void IRAM_ATTR alRecibir() { rxFlag = true; }

// =====================================================================
#if NODE_ID == 1
// ---------------------------- NODO 1: TERMICO ----------------------
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MAX31865.h>

#define RNOMINAL 100.0
#define RREF     430.0
// MAX31865_4WIRE con el puente corto en el bloque izquierdo (sin soldar).
// Usa MAX31865_3WIRE solo si modificaste los puentes de la placa.
#define PT_HILOS MAX31865_4WIRE

// Desfase de cada sonda medido en agua con hielo (°C que se RESTAN a la lectura)
static const float PT_OFFSET[3] = {0.0f, 0.0f, 0.0f};

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp(&Wire1);
Adafruit_MAX31865* pts[3];
bool ahtOk = false, bmpOk = false;

static bool leerPT(int i, float* t) {
  float v = pts[i]->temperature(RNOMINAL, RREF);
  uint8_t fallo = pts[i]->readFault();
  if (fallo) { pts[i]->clearFault(); return false; }
  v -= PT_OFFSET[i];
  if (v < -60.0f || v > 200.0f) return false;   // sin sonda/placa: la libreria devuelve ~ -242
  *t = v;
  return true;
}

void rolSetup() {
  vextOn(300);
  Wire1.begin(I2C_SDA, I2C_SCL);
  ahtOk = aht.begin(&Wire1);
  bmpOk = bmp.begin(0x76) || bmp.begin(0x77);
  pts[0] = new Adafruit_MAX31865(PT_CS1, PT_MOSI, PT_MISO, PT_SCK);
  pts[1] = new Adafruit_MAX31865(PT_CS2, PT_MOSI, PT_MISO, PT_SCK);
  pts[2] = new Adafruit_MAX31865(PT_CS3, PT_MOSI, PT_MISO, PT_SCK);
  for (int i = 0; i < 3; i++) pts[i]->begin(PT_HILOS);
  Serial.printf("Nodo 1 (termico): AHT20 %s, BMP280 %s\n", ahtOk ? "OK" : "NO", bmpOk ? "OK" : "NO");
}

void rolLoop() {}

void responder(const TramaPoll& p) {
  TramaTermico t = {};
  t.tipo = MSG_TERMICO;
  t.origen = NODE_ID;
  t.seq = p.seq;

  for (int i = 0; i < 3; i++) {
    float v;
    if (leerPT(i, &v)) {
      t.estado |= (1 << (2 + i));
      t.pt_x100[i] = (int16_t)lroundf(v * 100.0f);
    }
  }
  if (ahtOk) {
    sensors_event_t hum, temp;
    if (aht.getEvent(&hum, &temp)) {
      t.estado |= 0x01;
      t.tamb_x100 = (int16_t)lroundf(temp.temperature * 100.0f);
      t.hr_x100   = (uint16_t)lroundf(hum.relative_humidity * 100.0f);
    }
  }
  if (bmpOk) {
    float pa = bmp.readPressure();
    if (pa > 30000.0f && pa < 110000.0f) {
      t.estado |= 0x02;
      t.pres_x10 = (uint16_t)lroundf(pa / 10.0f);   // Pa / 10 = hPa * 10
    }
  }
  radio.transmit((uint8_t*)&t, sizeof(t));
  Serial.printf("Poll #%d -> estado=0x%02X\n", (int)p.seq, t.estado);
}

// =====================================================================
#elif NODE_ID == 2
// ---------------------------- NODO 2: RADIACION --------------------
#include <Wire.h>
#include <Adafruit_LTR390.h>

#define RS485_BAUD 4800   // piranometro PR-300AL-RA-N01
#define RS485_ADDR 1

#define INA_ADDR   0x40
#define INA_CANAL  1      // el canal que mide el panel
#define SHUNT_OHM  0.1f   // R100

Adafruit_LTR390 ltr;
bool ltrOk = false;
bool uvValido = false;
uint32_t uvsUltimo = 0;

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

static bool inaLeer(float* volts, float* mA) {
  int16_t shunt, bus;
  if (!ina16(0x01 + 2 * (INA_CANAL - 1), &shunt)) return false;
  if (!ina16(0x02 + 2 * (INA_CANAL - 1), &bus)) return false;
  *mA    = (shunt >> 3) * 0.04f / SHUNT_OHM;   // 40 uV por paso
  *volts = (bus >> 3) * 0.008f;                 // 8 mV por paso
  return true;
}

void rolSetup() {
  vextOn(300);
  Wire1.begin(I2C_SDA, I2C_SCL);
  ltrOk = ltr.begin(&Wire1);
  if (ltrOk) {
    ltr.setMode(LTR390_MODE_UVS);
    ltr.setGain(LTR390_GAIN_18);
    ltr.setResolution(LTR390_RESOLUTION_18BIT);
  }
  abrirRS485(RS485_BAUD);
  Serial.printf("Nodo 2 (radiacion): LTR390 %s\n", ltrOk ? "OK" : "NO");
}

void rolLoop() {
  // mantiene fresca la ultima lectura UV
  if (ltrOk && ltr.newDataAvailable()) {
    uvsUltimo = ltr.readUVS();
    uvValido = true;
  }
}

void responder(const TramaPoll& p) {
  TramaRadiacion t = {};
  t.tipo = MSG_RADIACION;
  t.origen = NODE_ID;
  t.seq = p.seq;

  uint16_t r = 0;
  if (leerRegistros(RS485_ADDR, 0x0000, 1, &r) == 0) {   // 0x0000 = radiacion solar, W/m2
    t.estado |= 0x01;
    t.irr_wm2 = r;
  }
  if (ltrOk && uvValido) {
    t.estado |= 0x02;
    float uvi = uvsUltimo / 575.0f;                      // 18x, 18 bits -> 575 cuentas por UVI
    t.uvi_x100 = (uint16_t)constrain(lroundf(uvi * 100.0f), 0L, 65535L);
  }
  float v, mA;
  if (inaLeer(&v, &mA)) {
    t.estado |= 0x04;
    t.vpanel_mV = (uint16_t)constrain(lroundf(v * 1000.0f), 0L, 65535L);
    t.ipanel_mA = (int16_t)constrain(lroundf(mA), -32768L, 32767L);
  }
  radio.transmit((uint8_t*)&t, sizeof(t));
  Serial.printf("Poll #%d -> estado=0x%02X irr=%d W/m2\n", (int)p.seq, t.estado, (int)t.irr_wm2);
}

// =====================================================================
#elif NODE_ID == 3
// ---------------------------- NODO 3: CULTIVO ----------------------
#include <DHT.h>

#define RS485_BAUD 9600   // sensor de suelo
#define RS485_ADDR 2

#define RELE_ACTIVO_BAJO   1       // 1 si el modulo de rele se activa con LOW
#define PULSOS_POR_LITRO   450.0f  // nominal del YF-S201; calibrar
#define RIEGO_DEFECTO_S    30      // si la orden no trae duracion
#define RIEGO_MAX_S        600     // limite duro: nunca se riega mas de esto por orden
#define SIN_ENLACE_MAX_MS  90000UL // si no hay consulta del gateway en este tiempo, se cierra

#if RELE_ACTIVO_BAJO
  #define NIVEL_ON  LOW
  #define NIVEL_OFF HIGH
#else
  #define NIVEL_ON  HIGH
  #define NIVEL_OFF LOW
#endif

DHT dht(PIN_DHT, DHT22);

volatile uint32_t pulsos = 0;
void IRAM_ATTR contarPulso() { pulsos++; }
uint32_t pulsosTotal = 0;
uint32_t tUltimoCaudal = 0;

bool valvulaAbierta = false;
uint32_t valvulaHasta = 0;     // millis() en que se cierra
uint32_t tUltimaConsulta = 0;
uint8_t ackId = 0;

void releOff() { digitalWrite(PIN_RELE, NIVEL_OFF); }
void releOn()  { digitalWrite(PIN_RELE, NIVEL_ON); }

void cerrarValvula(const char* motivo) {
  releOff();
  if (valvulaAbierta) Serial.printf("Valvula CERRADA (%s)\n", motivo);
  valvulaAbierta = false;
}

void abrirValvula(uint16_t durS) {
  if (durS == 0) durS = RIEGO_DEFECTO_S;
  if (durS > RIEGO_MAX_S) durS = RIEGO_MAX_S;
  releOn();
  valvulaAbierta = true;
  valvulaHasta = millis() + (uint32_t)durS * 1000UL;
  Serial.printf("Valvula ABIERTA por %u s\n", (unsigned)durS);
}

void rolSetup() {
  pinMode(PIN_RELE, OUTPUT);
  releOff();                       // lo primero: arranca siempre cerrada
  vextOn(1000);
  dht.begin();
  pinMode(PIN_FLUJO, INPUT);       // con 5 V o mas, divisor 10k/20k antes del pin
  attachInterrupt(digitalPinToInterrupt(PIN_FLUJO), contarPulso, RISING);
  abrirRS485(RS485_BAUD);
  tUltimoCaudal = millis();
  tUltimaConsulta = millis();
  Serial.println("Nodo 3 (cultivo) listo, valvula cerrada");
}

void rolLoop() {
  if (valvulaAbierta) {
    if ((int32_t)(millis() - valvulaHasta) >= 0) cerrarValvula("tiempo cumplido");
    else if (millis() - tUltimaConsulta > SIN_ENLACE_MAX_MS) cerrarValvula("sin enlace con el gateway");
  }
}

void responder(const TramaPoll& p) {
  tUltimaConsulta = millis();

  // 1) Orden de la valvula (primero, para que la respuesta ya refleje el estado)
  if (p.cmd != CMD_NADA && p.cmd_id != 0 && p.cmd_id != ackId) {
    ackId = p.cmd_id;
    if (p.cmd == CMD_ABRIR) abrirValvula(p.dur_s);
    else if (p.cmd == CMD_CERRAR) cerrarValvula("orden del gateway");
  }

  TramaCultivo t = {};
  t.tipo = MSG_CULTIVO;
  t.origen = NODE_ID;
  t.seq = p.seq;
  t.valvula = valvulaAbierta ? 1 : 0;
  t.ack_id = ackId;

  // 2) Suelo (RS485)
  uint16_t r[8];
  if (leerRegistros(RS485_ADDR, 0x0000, 8, r) == 0) {
    t.estado |= 0x01;
    t.suelo_temp_x10 = (int16_t)r[0];
    t.suelo_hum_x10  = r[1];
    t.ec = r[2];
    t.sal = r[3];
    t.n = r[4];
    t.p = r[5];
    t.k = r[6];
    t.ph_x10 = r[7];
  }

  // 3) DHT22
  float h = dht.readHumidity();
  float tc = dht.readTemperature();
  if (!isnan(h) && !isnan(tc)) {
    t.estado |= 0x02;
    t.dht_temp_x10 = (int16_t)lroundf(tc * 10.0f);
    t.dht_hr_x10   = (uint16_t)lroundf(h * 10.0f);
  }

  // 4) Flujo: pulsos desde la consulta anterior
  noInterrupts();
  uint32_t pl = pulsos;
  pulsos = 0;
  interrupts();
  uint32_t ahora = millis();
  float dt = (ahora - tUltimoCaudal) / 1000.0f;
  tUltimoCaudal = ahora;
  pulsosTotal += pl;
  float lmin = (dt > 0.0f) ? (pl / PULSOS_POR_LITRO) / dt * 60.0f : 0.0f;
  t.caudal_x100 = (uint16_t)constrain(lroundf(lmin * 100.0f), 0L, 65535L);
  t.volumen_ml  = (uint32_t)(pulsosTotal * 1000.0f / PULSOS_POR_LITRO);

  radio.transmit((uint8_t*)&t, sizeof(t));
  Serial.printf("Poll #%d -> estado=0x%02X valvula=%d caudal=%.2f L/min\n",
                (int)p.seq, t.estado, (int)t.valvula, lmin);
}

#else
  #error "NODE_ID debe ser 1, 2 o 3"
#endif

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  rolSetup();

  int st = radio.begin(FREQ_MHZ, 125.0, 9, 7, 0x12, 10, 8, 1.8);
  radio.setDio2AsRfSwitch(true);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("Error LoRa: %d\n", st);
    while (true) { rolLoop(); delay(100); }   // en el nodo 3 el temporizador de la valvula sigue activo
  }
  radio.setPacketReceivedAction(alRecibir);
  radio.startReceive();
  Serial.printf("Nodo %d listo\n", NODE_ID);
}

void loop() {
  rolLoop();
  if (!rxFlag) return;
  rxFlag = false;

  uint8_t buf[64];
  size_t n = radio.getPacketLength();
  int st = radio.readData(buf, 0);

  if (st == RADIOLIB_ERR_NONE && n == sizeof(TramaPoll)) {
    TramaPoll p;
    memcpy(&p, buf, sizeof(p));
    if (p.tipo == MSG_POLL && p.destino == NODE_ID) {
      delay(50);            // darle tiempo al gateway de pasar a recepcion
      responder(p);
    }
  }
  radio.startReceive();
}
