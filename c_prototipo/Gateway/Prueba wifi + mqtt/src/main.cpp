#include <Arduino.h>
#include "conexiones.h"
#include "lcdplus.h"
#include "web_portal.h"

unsigned long apStartTime = 0;
const unsigned long apDuration = 180000; // 3 min
unsigned long lastPublish = 0;
const unsigned long publishInterval = 30000; // 30s
unsigned long lastLCD = 0;
const unsigned long lcdInterval = 2000; // 2s (más rápido para mejor feedback)

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
      
      // Publicar estado
      if (millis() - lastPublish >= publishInterval) {
        publishStatus();
        lastPublish = millis();
      }
    }
  }
  
  // Actualizar LCD periódicamente (solo si está en modo NORMAL)
  if (millis() - lastLCD >= lcdInterval) {
    actualizarLCD();
    lastLCD = millis();
  }
  
  delay(10);
}