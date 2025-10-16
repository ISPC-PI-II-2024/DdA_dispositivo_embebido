/* 
/gateway_mqtt/
├── main.cpp
├── conexiones.h
├── conexiones.cpp
├── lcdplus.h
├── lcdplus.cpp
├── web_portal.h
├── web_portal.cpp   
└── WebSocketClient.h           
*/
// main.cpp
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "conexiones.h"
#include "lcdplus.h"
#include "web_portal.h"

unsigned long apStartTime = 0;
const unsigned long apDuration = 180000; // 3 min
unsigned long lastPublish = 0;
const unsigned long publishInterval = 30000; // 30s
unsigned long lastLCD = 0;
const unsigned long lcdInterval = 5000; // 5s
unsigned long lastMQTTCheck = 0;
const unsigned long mqttCheckInterval = 5000; // Verificar MQTT cada 5s

void setup() {
  Serial.begin(115200);
  delay(100);
  
  initLCD();
  getLCD().clear(); 
  getLCD().print("Iniciando...");
  
  initWiFi();
  
  if (wifiConfigured) {
    apMode = false;
    conectarWifi();
    
    // Esperar conexión WiFi
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi conectado!");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      initMQTT();
    }
  } else {
    iniciarAP();
    setupWebServer();
    apStartTime = millis();
  }
}

void loop() {
  if (apMode) {
    handleWebRequests();
    if (millis() - apStartTime >= apDuration) {
      WiFi.softAPdisconnect(true);
      apMode = false;
      if (!wifiConfigured) {
        getLCD().clear(); 
        getLCD().print("Sin config WiFi");
      }
    }
  } else {
    handleWiFiReconnect();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Loop de WebSocket
      webSocket.loop();
      
      // Loop de MQTT
      mqttClient.loop();
      
      // Verificar conexión MQTT periódicamente
      if (millis() - lastMQTTCheck >= mqttCheckInterval) {
        if (!mqttClient.connected()) {
          reconnectMQTT();
        }
        lastMQTTCheck = millis();
      }
      
      // Publicar estado
      if (millis() - lastPublish >= publishInterval) {
        publishStatus();
        lastPublish = millis();
      }
    }
  }
  
  // Actualizar LCD
  if (millis() - lastLCD >= lcdInterval) {
    actualizarLCD();
    lastLCD = millis();
  }
  
  delay(10);
}