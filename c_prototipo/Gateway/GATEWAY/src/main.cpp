// main.cpp
#include <Arduino.h>
#include "conexiones.h"
#include "lcdplus.h"
#include "web_portal.h"
#include "lora_manager.h"

unsigned long apStartTime = 0;
const unsigned long apDuration = 180000; // 3 min
unsigned long lastPublishGateway = 0;
unsigned long lastPublishEndpoints = 0;
unsigned long lastPublishSensors = 0;
const unsigned long publishIntervalGateway = 30000; // 30s - Estado gateway
const unsigned long publishIntervalEndpoints = 60000; // 60s - Estado endpoints
const unsigned long publishIntervalSensors = 60000; // 60s - Datos sensores
unsigned long lastLCD = 0;
const unsigned long lcdInterval = 2000; // 2s
unsigned long lastLoraUpdate = 0;
const unsigned long loraUpdateInterval = 45000; // 45s - Polling endpoints

void mqttLoop();

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Gateway MQTT ESP32 ===\n");
  
  // Inicializar LCD y mostrar splash
  initLCD();
  lcdMode = LCDMode::NORMAL; 
  mostrarMensajeLCD("Gateway MQTT", "Iniciando...", 2000);
  delay(2000);
  
  // Inicializar LoRa
  mostrarMensajeLCD("LoRa:", "Iniciando...", 0);
  if (initLora()) {
    mostrarMensajeLCD("LoRa: OK", "433 MHz", 2000);
  } else {
    mostrarMensajeLCD("LoRa: ERROR", "Sin modulo", 3000);
  }
  delay(2000);
  
  initWiFi();
  
  if (wifiConfigured) {
    // WiFi configurado - intentar conectar
    apMode = false;
    mostrarMensajeLCD("WiFi Config: SI", "Conectando...", 0);
    
    conectarWifi();
    
    // Esperar conexión WiFi con feedback
    Serial.print("Conectando WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      
      // Actualizar LCD cada 5 intentos
      if (attempts % 5 == 0) {
        String dots = "";
        for(int i = 0; i < (attempts/5) % 4; i++) dots += ".";
        mostrarMensajeLCD("Conectando WiFi", dots, 0);
      }
      attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ WiFi conectado!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      
      mostrarMensajeLCD("WiFi: OK", "Init MQTT...", 2000);
      delay(2000);
      
      // Inicializar MQTT/WebSocket
      initMQTT();
      
      // Esperar WebSocket
      Serial.println("Esperando WebSocket...");
      mostrarMensajeLCD("MQTT: Esperando", "Conexion...", 2000);
      delay(2000);
      
      // Volver a modo normal
      volverModoNormal();
      
    } else {
      Serial.println("❌ WiFi falló");
      mostrarMensajeLCD("WiFi: ERROR", "Sin conexion", 3000);
      delay(3000);
    }
  } else {
    // Sin configuración - iniciar AP
    mostrarMensajeLCD("WiFi Config: NO", "Iniciando AP...", 2000);
    delay(2000);
    
    iniciarAP();
    setupWebServer();
    apStartTime = millis();
    
    volverModoNormal();
  }
}

void loop() {
  // Verificar botón de reset (SIEMPRE)
  checkResetButton();
  
  // Loop LoRa pasivo (escuchar mensajes no solicitados)
  loraLoop();
  
  if (apMode) {
    handleWebRequests();
    
    // Timeout del AP
    if (millis() - apStartTime >= apDuration) {
      WiFi.softAPdisconnect(true);
      apMode = false;
      if (!wifiConfigured) {
        mostrarMensajeLCD("AP Timeout", "Sin config WiFi", 0);
      }
    }
  } else {
    handleWiFiReconnect();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Loop de WebSocket
      webSocket.loop();
      
      // Loop MQTT (ping)
      mqttLoop();
      
      // Actualizar endpoints vía LoRa (polling)
      if (millis() - lastLoraUpdate >= loraUpdateInterval) {
        if (loraInicializado) {
          actualizarTodosEndpoints();
        }
        lastLoraUpdate = millis();
      }
      
      // Publicar datos solo si MQTT está conectado
      if (mqttClient && mqttClient->connected()) {
        
        // Tópico 1: gateway/gateway (estado del gateway)
        if (millis() - lastPublishGateway >= publishIntervalGateway) {
          String json = getGatewayStatusJSON();
          String topic = "gateway/gateway";
          mqttClient->publish(topic.c_str(), json.c_str());
          Serial.println("📤 [gateway/gateway] " + json);
          lastPublishGateway = millis();
        }
        
        // Tópico 2: gateway/endpoint (estado de endpoints)
        if (millis() - lastPublishEndpoints >= publishIntervalEndpoints) {
          String json = getEndpointsStatusJSON();
          String topic = "gateway/endpoint";
          mqttClient->publish(topic.c_str(), json.c_str());
          Serial.println("📤 [gateway/endpoint] " + json);
          lastPublishEndpoints = millis();
        }
        
        // Tópico 3: gateway/sensor (datos de sensores)
        if (millis() - lastPublishSensors >= publishIntervalSensors) {
          String json = getSensorsDataJSON();
          String topic = "gateway/sensor";
          mqttClient->publish(topic.c_str(), json.c_str());
          Serial.println("📤 [gateway/sensor] " + json);
          lastPublishSensors = millis();
        }
      }
      
      // Limpiar endpoints inactivos
      limpiarEndpointsInactivos();
    }
  }
  
  // Actualizar LCD periódicamente (solo si está en modo NORMAL)
  if (millis() - lastLCD >= lcdInterval) {
    actualizarLCD();
    lastLCD = millis();
  }
  
  delay(10);
}