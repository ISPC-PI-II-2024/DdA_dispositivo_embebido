// conexiones.cpp
#include "conexiones.h"
#include "WebSocketClient.h"
#include <time.h>


// Configuración
const char* gatewayId = "Gat_01";
const char* mqtt_server = "mqtt.ispciot.org";
const uint16_t mqtt_port = 80;
const char* mqtt_path = "/mqtt";

// Variables globales
String ssid = "";
String password = "";
bool wifiConfigured = false;
bool apMode = false;
bool mqttConnected = false;
Preferences preferences;
WebSocketsClient webSocket;
WebSocketClient* wsClient = nullptr;
PubSubClient mqttClient;

// === UTILS ===
String obtenerHoraArgentina() {
  static bool timeSet = false;
  if (!timeSet) {
    configTime(-3 * 3600, 0, "pool.ntp.org");
    timeSet = true;
  }
  struct tm t;
  if (!getLocalTime(&t, 5000)) return "Hora no disp.";
  char buf[20];
  strftime(buf, sizeof(buf), "%H:%M %d/%m", &t);
  return String(buf);
}

String calidadSenalWifi(int rssi) {
  if (rssi >= -50) return "Excelente";
  if (rssi >= -60) return "Buena";
  if (rssi >= -70) return "Regular";
  return "Debil";
}

// === WEBSOCKET EVENT ===
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket desconectado");
      mqttConnected = false;
      break;
      
    case WStype_CONNECTED:
      Serial.println("WebSocket conectado");
      break;
      
    case WStype_TEXT:
      Serial.printf("WS recibido (texto): %s\n", payload);
      break;
      
    case WStype_BIN:
      // Datos binarios de MQTT
      if (wsClient) {
        wsClient->feedData(payload, length);
      }
      break;
  }
}

// === MQTT CALLBACK ===
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println("MQTT recibido [" + String(topic) + "]: " + msg);
}

// === WIFI ===
void initWiFi() {
  preferences.begin("wifi_cfg", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  wifiConfigured = (ssid.length() > 0);
}

void conectarWifi() {
  if (ssid == "") return;
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Conectando a WiFi: " + ssid);
}

void iniciarAP() {
  String apName = "GatewayMQTT_" + String(gatewayId);
  WiFi.softAP(apName.c_str());
  apMode = true;
  Serial.println("AP iniciado: " + apName);
}

void handleWiFiReconnect() {
  static uint32_t lastTry = 0;
  if (wifiConfigured && WiFi.status() != WL_CONNECTED && millis() - lastTry > 10000) {
    conectarWifi();
    lastTry = millis();
  }
}

// === MQTT ===
void initMQTT() {
  // Conectar WebSocket
  webSocket.begin(mqtt_server, mqtt_port, mqtt_path);
  webSocket.onEvent(webSocketEvent);
  
  // Crear wrapper de cliente
  wsClient = new WebSocketClient(&webSocket);
  
  // Configurar MQTT con el wrapper
  mqttClient.setClient(*wsClient);
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  Serial.println("MQTT/WebSocket inicializado");
}

void reconnectMQTT() {
  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Intentando conexión MQTT...");
    
    if (mqttClient.connect(gatewayId)) {
      mqttConnected = true;
      Serial.println("✅ conectado");
      // Suscribirse a tópicos
      String cmdTopic = "gateway/" + String(gatewayId) + "/cmd";
      mqttClient.subscribe(cmdTopic.c_str());
      Serial.println("Suscrito a: " + cmdTopic);
    } else {
      mqttConnected = false;
      Serial.print("❌ falló, rc=");
      Serial.println(mqttClient.state());
    }
  }
}

void publishStatus() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
    return;
  }
  
  String payload = "{";
  payload += "\"gateway_id\":\"" + String(gatewayId) + "\",";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"quality\":\"" + calidadSenalWifi(WiFi.RSSI()) + "\",";
  payload += "\"timestamp\":\"" + obtenerHoraArgentina() + "\"";
  payload += "}";
  
  String topic = "gateway/" + String(gatewayId) + "/status";
  
  if (mqttClient.publish(topic.c_str(), payload.c_str())) {
    Serial.println("✅ Publicado: " + topic);
  } else {
    Serial.println("❌ Fallo al publicar");
  }
}