#include <Arduino.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <mqtt_client.h>
#include <string.h>
#include "common.h"
#include "secrets.h"

SX1262 radio = new Module(8, 14, 12, 13);

volatile bool rxFlag = false;
void IRAM_ATTR alRecibir() { rxFlag = true; }

const uint8_t NODOS[] = {NODO_TERMICO, NODO_RADIACION, NODO_CULTIVO};
uint16_t seq = 0;

void publicar(uint8_t id, const char* sufijo, const char* json);

// ---------- Orden pendiente para la electrovalvula (nodo 3) ----------
#define DUR_PRUEBA_S   10   // duracion de la apertura con la tecla 'a' en el monitor serie
#define MAX_INTENTOS_CMD 5
uint8_t  pendCmd = CMD_NADA;
uint8_t  pendId = 0;
uint16_t pendDur = 0;
uint8_t  pendIntentos = 0;
uint8_t  ultimoId = 0;

void nuevaOrden(uint8_t cmd, uint16_t dur) {
  ultimoId++;
  if (ultimoId == 0) ultimoId = 1;   // el 0 significa "sin orden"
  pendCmd = cmd;
  pendId = ultimoId;
  pendDur = dur;
  pendIntentos = 0;
  Serial.printf(">> Orden #%d: %s%s\n", (int)pendId, cmd == CMD_ABRIR ? "ABRIR valvula" : "CERRAR valvula",
                cmd == CMD_ABRIR ? " (en la proxima consulta al nodo 3)" : "");
  char ev[96];
  snprintf(ev, sizeof(ev), "{\"nodo\":%d,\"orden\":%d,\"estado\":\"recibida\"}", (int)NODO_CULTIVO, (int)pendId);
  publicar(NODO_CULTIVO, "evento", ev);
}

// Buzon: el hilo de MQTT deja aqui la orden y el loop principal la toma
#define TOPIC_CMD "agri/nodo/3/cmd"
static volatile uint8_t  mailCmd = CMD_NADA;
static volatile uint16_t mailDur = 0;

// Ordenes de MQTT y teclas del monitor serie: 'a' = abrir (DUR_PRUEBA_S s), 'c' = cerrar
void leerTeclas() {
  if (mailCmd != CMD_NADA) {
    uint8_t c = mailCmd;
    uint16_t d = mailDur;
    mailCmd = CMD_NADA;
    nuevaOrden(c, d);
  }
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'a') nuevaOrden(CMD_ABRIR, DUR_PRUEBA_S);
    else if (c == 'c') nuevaOrden(CMD_CERRAR, 0);
  }
}

// ---------- Wi-Fi (diagnostico) ----------
void wifiEvento(WiFiEvent_t evento, WiFiEventInfo_t info) {
  if (evento == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.printf("WiFi desconectado, razon=%d\n", info.wifi_sta_disconnected.reason);
  } else if (evento == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    Serial.printf("WiFi conectado, IP=%s\n", WiFi.localIP().toString().c_str());
  }
}

// ---------- MQTT sobre WebSocket ----------
static esp_mqtt_client_handle_t mqttClient = nullptr;
static volatile bool mqttConectado = false;
static bool mqttIniciado = false;
static char mqttUri[96];

// Orden recibida por MQTT. Formato:  {"accion":"abrir","seg":60}  o  {"accion":"cerrar"}
static void procesarComando(esp_mqtt_event_handle_t ev) {
  if (ev->topic_len != (int)strlen(TOPIC_CMD) || strncmp(ev->topic, TOPIC_CMD, ev->topic_len) != 0) return;
  if (ev->data_len <= 0 || ev->data_len != ev->total_data_len || ev->data_len > 120) return;

  char msg[128];
  memcpy(msg, ev->data, ev->data_len);
  msg[ev->data_len] = 0;
  Serial.printf("MQTT -> gateway (%s): %s\n", TOPIC_CMD, msg);

  if (strstr(msg, "cerrar")) {          // cerrar tiene prioridad
    mailDur = 0;
    mailCmd = CMD_CERRAR;
  } else if (strstr(msg, "abrir")) {
    long seg = 0;
    const char* k = strstr(msg, "\"seg\"");
    if (k) {
      k = strchr(k, ':');
      if (k) seg = atol(k + 1);
    }
    if (seg < 0) seg = 0;
    if (seg > 65535) seg = 65535;
    mailDur = (uint16_t)seg;
    mailCmd = CMD_ABRIR;
  }
}

static void mqttEvento(void*, esp_event_base_t, int32_t id, void* datos) {
  esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)datos;
  switch (id) {
    case MQTT_EVENT_CONNECTED:
      mqttConectado = true;
      Serial.println("MQTT/WS conectado");
      esp_mqtt_client_publish(mqttClient, "agri/gateway/estado", "online", 0, 0, 0);
      Serial.println("Suscrito a " TOPIC_CMD);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
      esp_mqtt_client_subscribe_single(mqttClient, TOPIC_CMD, 0);
#else
      esp_mqtt_client_subscribe(mqttClient, TOPIC_CMD, 0);
#endif
      break;
    case MQTT_EVENT_DISCONNECTED:
      mqttConectado = false;
      Serial.println("MQTT/WS desconectado");
      break;
    case MQTT_EVENT_DATA:
      if (ev) procesarComando(ev);
      break;
    case MQTT_EVENT_ERROR:
      Serial.println("MQTT/WS error");
      break;
  }
}

void iniciarMqtt() {
  if (mqttIniciado || WiFi.status() != WL_CONNECTED) return;

  snprintf(mqttUri, sizeof(mqttUri), "ws://%s:%d%s", MQTT_HOST, MQTT_PORT, MQTT_PATH);
  Serial.printf("WiFi OK (%s). Conectando a %s\n",
                WiFi.localIP().toString().c_str(), mqttUri);

  esp_mqtt_client_config_t cfg = {};
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  cfg.broker.address.uri = mqttUri;
  cfg.credentials.client_id = "gateway-01";
  cfg.session.keepalive = 30;
  cfg.network.reconnect_timeout_ms = 5000;
#else
  cfg.uri = mqttUri;
  cfg.client_id = "gateway-01";
  cfg.keepalive = 30;
  cfg.reconnect_timeout_ms = 5000;
#endif

  mqttClient = esp_mqtt_client_init(&cfg);
  esp_mqtt_client_register_event(mqttClient, MQTT_EVENT_ANY, mqttEvento, nullptr);
  esp_mqtt_client_start(mqttClient);
  mqttIniciado = true;
}

void publicar(uint8_t id, const char* sufijo, const char* json) {
  Serial.println(json);
  if (!mqttConectado) return;
  char topic[48];
  snprintf(topic, sizeof(topic), "agri/nodo/%d/%s", id, sufijo);
  esp_mqtt_client_publish(mqttClient, topic, json, 0, 0, 0);
}

void publicarError(uint8_t id, const char* motivo) {
  char json[80];
  snprintf(json, sizeof(json), "{\"nodo\":%d,\"error\":\"%s\"}", id, motivo);
  publicar(id, "error", json);
}

// ---------- Constructor de JSON (solo agrega los campos que el nodo pudo medir) ----------
static char jbuf[512];
static size_t jlen = 0;

static void jIni(int nodo, int seqN) {
  jlen = snprintf(jbuf, sizeof(jbuf), "{\"nodo\":%d,\"seq\":%d", nodo, seqN);
}

static void jNum(const char* clave, double valor, int decimales) {
  if (jlen >= sizeof(jbuf) - 40) return;   // proteccion contra desbordamiento
  jlen += snprintf(jbuf + jlen, sizeof(jbuf) - jlen, ",\"%s\":%.*f", clave, decimales, valor);
}

static const char* jFin(float rssi, float snr) {
  jNum("rssi", rssi, 1);
  jNum("snr", snr, 1);
  if (jlen < sizeof(jbuf) - 2) { jbuf[jlen++] = '}'; jbuf[jlen] = 0; }
  return jbuf;
}

// ---------- Decodificacion de tramas ----------
// Devuelve true si la respuesta era del nodo consultado.
bool procesarRespuesta(uint8_t id, const uint8_t* b, size_t n, float rssi, float snr) {
  if (n < 2 || b[1] != id) return false;

  if (b[0] == MSG_TERMICO && n == sizeof(TramaTermico)) {
    TramaTermico t;
    memcpy(&t, b, sizeof(t));
    Serial.printf("[nodo %d estado=0x%02X]\n", id, t.estado);
    if (t.estado == 0) { publicarError(id, "sin_sensores"); return true; }
    jIni(t.origen, t.seq);
    for (int i = 0; i < 3; i++) {
      if (t.estado & (1 << (2 + i))) {
        char k[8];
        snprintf(k, sizeof(k), "pt%d", i + 1);
        jNum(k, t.pt_x100[i] / 100.0, 2);
      }
    }
    if (t.estado & 0x01) { jNum("temp_amb", t.tamb_x100 / 100.0, 2); jNum("hum_amb", t.hr_x100 / 100.0, 1); }
    if (t.estado & 0x02) jNum("presion", t.pres_x10 / 10.0, 1);
    publicar(id, "telemetria", jFin(rssi, snr));
    return true;
  }

  if (b[0] == MSG_RADIACION && n == sizeof(TramaRadiacion)) {
    TramaRadiacion t;
    memcpy(&t, b, sizeof(t));
    Serial.printf("[nodo %d estado=0x%02X]\n", id, t.estado);
    if (t.estado == 0) { publicarError(id, "sin_sensores"); return true; }
    jIni(t.origen, t.seq);
    if (t.estado & 0x01) jNum("irr", t.irr_wm2, 0);
    if (t.estado & 0x02) jNum("uvi", t.uvi_x100 / 100.0, 2);
    if (t.estado & 0x04) {
      double v = t.vpanel_mV / 1000.0;
      jNum("v_panel", v, 3);
      jNum("i_panel", t.ipanel_mA, 1);
      jNum("p_panel", v * t.ipanel_mA, 1);   // mW
    }
    publicar(id, "telemetria", jFin(rssi, snr));
    return true;
  }

  if (b[0] == MSG_CULTIVO && n == sizeof(TramaCultivo)) {
    TramaCultivo t;
    memcpy(&t, b, sizeof(t));
    Serial.printf("[nodo %d estado=0x%02X valvula=%d ack=%d]\n", id, t.estado, (int)t.valvula, (int)t.ack_id);

    // La orden pendiente se da por recibida cuando el nodo devuelve su identificador
    if (pendCmd != CMD_NADA && t.ack_id == pendId) {
      Serial.printf(">> Orden #%d confirmada por el nodo (valvula %s)\n", (int)pendId, t.valvula ? "ABIERTA" : "CERRADA");
      pendCmd = CMD_NADA;
      char ev[96];
      snprintf(ev, sizeof(ev), "{\"nodo\":%d,\"orden\":%d,\"valvula\":%d}", (int)id, (int)t.ack_id, (int)t.valvula);
      publicar(id, "evento", ev);
    }

    jIni(t.origen, t.seq);
    if (t.estado & 0x01) {
      jNum("suelo_temp", t.suelo_temp_x10 / 10.0, 1);
      jNum("suelo_hum", t.suelo_hum_x10 / 10.0, 1);
      jNum("ec", t.ec, 0);
      jNum("sal", t.sal, 0);
      jNum("nitrogeno", t.n, 0);
      jNum("fosforo", t.p, 0);
      jNum("potasio", t.k, 0);
      jNum("ph", t.ph_x10 / 10.0, 1);
    }
    if (t.estado & 0x02) { jNum("dht_temp", t.dht_temp_x10 / 10.0, 1); jNum("dht_hum", t.dht_hr_x10 / 10.0, 1); }
    jNum("caudal", t.caudal_x100 / 100.0, 2);        // L/min
    jNum("volumen", t.volumen_ml / 1000.0, 3);       // L acumulados
    jNum("valvula", t.valvula, 0);
    publicar(id, "telemetria", jFin(rssi, snr));
    return true;
  }
  return false;
}

// ---------- LoRa ----------
void consultar(uint8_t id) {
  TramaPoll p = {};
  p.tipo = MSG_POLL;
  p.destino = id;
  p.seq = seq++;
  if (id == NODO_CULTIVO && pendCmd != CMD_NADA) {
    if (pendIntentos >= MAX_INTENTOS_CMD) {
      Serial.printf(">> Orden #%d descartada: el nodo no la confirmo\n", (int)pendId);
      pendCmd = CMD_NADA;
    } else {
      p.cmd = pendCmd;
      p.cmd_id = pendId;
      p.dur_s = pendDur;
      pendIntentos++;
    }
  }
  radio.transmit((uint8_t*)&p, sizeof(p));

  rxFlag = false;
  radio.startReceive();

  uint32_t t0 = millis();
  while (millis() - t0 < 3000) {
    if (!rxFlag) { delay(5); continue; }
    rxFlag = false;

    uint8_t buf[64];
    size_t n = radio.getPacketLength();
    int st = radio.readData(buf, 0);
    if (st == RADIOLIB_ERR_NONE && procesarRespuesta(id, buf, n, radio.getRSSI(), radio.getSNR())) {
      return;
    }
    radio.startReceive();
  }
  publicarError(id, "sin_respuesta");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  int st = radio.begin(FREQ_MHZ, 125.0, 9, 7, 0x12, 10, 8, 1.8);
  radio.setDio2AsRfSwitch(true);
  if (st != RADIOLIB_ERR_NONE) {
    Serial.printf("Error LoRa: %d\n", st);
    while (true) delay(1000);
  }
  radio.setPacketReceivedAction(alRecibir);

  WiFi.onEvent(wifiEvento);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Gateway listo. Teclas: 'a' = abrir valvula (10 s), 'c' = cerrar");
}

void loop() {
  iniciarMqtt();   // arranca el cliente MQTT cuando el Wi-Fi esta listo

  for (uint8_t id : NODOS) {
    leerTeclas();
    consultar(id);
    delay(300);
  }
  leerTeclas();
  delay(6000);
}