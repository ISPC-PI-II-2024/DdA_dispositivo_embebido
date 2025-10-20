// conexiones.cpp
#include "conexiones.h"
#include "mqtt_manual.h"
#include "lcdplus.h"
#include <time.h>

// Configuración
const char* gatewayId = "Gat_01";
const char* mqtt_server = "mqtt.ispciot.org";
const uint16_t mqtt_port = 80;
const char* mqtt_path = "/mqtt";

// ✅ CAMBIO: Usar GPIO 33 en lugar de GPIO 34
// GPIO 33 tiene resistencias pull-up/pull-down internas
const int RESET_BUTTON_PIN = 33;  

// Variables globales
String ssid = "";
String password = "";
bool wifiConfigured = false;
bool apMode = false;
bool mqttConnected = false;
Preferences preferences;
WebSocketsClient webSocket;
MQTTManual* mqttClient = nullptr;

unsigned long lastPing = 0;
const unsigned long pingInterval = 30000;  // Ping cada 30s

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
      Serial.println("❌ WebSocket desconectado");
      mqttConnected = false;
      if (mqttClient) {
        mqttClient->setConnected(false);
      }
      break;
      
    case WStype_CONNECTED:
      Serial.println("✅ WebSocket conectado");
      
      // Enviar MQTT CONNECT
      if (mqttClient) {
        mqttClient->connect();
      }
      break;
      
    case WStype_TEXT:
      Serial.printf("📝 WS texto: %s\n", payload);
      break;
      
    case WStype_BIN:
      // Procesar mensaje MQTT
      if (mqttClient) {
        mqttClient->processMessage(payload, length);
        
        // Si acabamos de conectar, suscribirse
        if (mqttClient->connected() && !mqttConnected) {
          mqttConnected = true;
          String cmdTopic = "gateway/" + String(gatewayId) + "/cmd";
          mqttClient->subscribe(cmdTopic.c_str());
        }
      }
      break;
      
    case WStype_ERROR:
      Serial.println("❌ WebSocket ERROR");
      break;
      
    case WStype_PING:
      Serial.println("🏓 PING");
      break;
      
    case WStype_PONG:
      Serial.println("🏓 PONG");
      break;
  }
}

// === WIFI ===
void initWiFi() {
  // Configurar botón de reset con PULL-DOWN
  // Reposo: LOW (no presionado)
  // Activo: HIGH (presionado - conecta a 3.3V)
  pinMode(RESET_BUTTON_PIN, INPUT_PULLDOWN);
  
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

void clearWiFiConfig() {
  preferences.begin("wifi_cfg", false);
  preferences.clear();
  preferences.end();
  wifiConfigured = false;
  ssid = "";
  password = "";
  Serial.println("✅ Configuración WiFi borrada");
}

// === BOTÓN RESET MEJORADO ===
void checkResetButton() {
  static unsigned long buttonPressStart = 0;
  static bool buttonPressed = false;
  static int lastSecondsShown = 0;
  const unsigned long HOLD_TIME = 3000;  // 3 segundos presionado

  // Con PULL-DOWN: HIGH = presionado, LOW = no presionado
  bool buttonState = digitalRead(RESET_BUTTON_PIN) == HIGH;

  if (buttonState && !buttonPressed) {
    // Botón recién presionado
    buttonPressed = true;
    buttonPressStart = millis();
    lastSecondsShown = 0;

    lcdMode = LCDMode::RESET_PROMPT;
    mostrarMensajeLCD("Mantener 3 seg", "para Reset WiFi", 0);
  }
  else if (!buttonState && buttonPressed) {
    // Botón soltado
    unsigned long pressDuration = millis() - buttonPressStart;
    buttonPressed = false;

    if (pressDuration >= HOLD_TIME) {
      // Ejecutar reset
      lcdMode = LCDMode::RESET_CONFIRM;
      mostrarMensajeLCD("Borrando WiFi", "Reiniciando...", 2000);

      clearWiFiConfig();
      delay(2000);
      ESP.restart();
    } else {
      // Cancelado
      lcdMode = LCDMode::RESET_CANCEL;
      mostrarMensajeLCD("Cancelado", "", 1500);
    }
  }
  else if (buttonState && buttonPressed) {
    // Botón mantenido: mostrar progreso
    unsigned long pressDuration = millis() - buttonPressStart;
    int seconds = (pressDuration / 1000) + 1;
    if (seconds > 3) seconds = 3;

    // Solo actualizar si cambió el segundo
    if (seconds != lastSecondsShown) {
      lastSecondsShown = seconds;
      
      lcdMode = LCDMode::RESET_PROGRESS;
      
      String line1 = "Reset WiFi: " + String(seconds) + "/3";
      
      // Crear barra de progreso
      int progress = (pressDuration * 100) / HOLD_TIME;
      if (progress > 100) progress = 100;
      int bars = (progress * 16) / 100;
      
      String line2 = "";
      for (int i = 0; i < 16; i++) {
        if (i < bars) {
          line2 += (char)255;  // Bloque sólido
        } else {
          line2 += "-";
        }
      }
      
      mostrarMensajeLCD(line1, line2, 0);
    }
  }
}

// === MQTT ===
void initMQTT() {
  // Crear cliente MQTT manual
  mqttClient = new MQTTManual(&webSocket, gatewayId);
  
  // Configurar WebSocket
  webSocket.begin(mqtt_server, mqtt_port, mqtt_path);
  webSocket.setExtraHeaders("Sec-WebSocket-Protocol: mqtt");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  
  Serial.println("MQTT/WebSocket inicializado");
}

void publishStatus() {
  if (!mqttClient || !mqttClient->connected()) {
    return;
  }
  
  // Construir payload JSON
  String payload = "{";
  payload += "\"gateway_id\":\"" + String(gatewayId) + "\",";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"quality\":\"" + calidadSenalWifi(WiFi.RSSI()) + "\",";
  payload += "\"timestamp\":\"" + obtenerHoraArgentina() + "\"";
  payload += "}";
  
  String topic = "gateway/" + String(gatewayId) + "/status";
  mqttClient->publish(topic.c_str(), payload.c_str());
}

// Llamar desde loop para mantener conexión
void mqttLoop() {
  // Enviar PING periódicamente
  if (mqttClient && mqttClient->connected() && millis() - lastPing > pingInterval) {
    mqttClient->ping();
    lastPing = millis();
  }
}