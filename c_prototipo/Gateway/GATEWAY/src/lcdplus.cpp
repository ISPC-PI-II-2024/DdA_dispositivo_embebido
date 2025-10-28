// lcdplus.cpp
#include "lcdplus.h"
#include "conexiones.h"
#include "lora_manager.h"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

// Instancia del LCD (dirección I2C: 0x27, 20 columnas, 4 filas)
static LiquidCrystal_I2C lcd(0x27, 20, 4);

void initLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  Serial.println("LCD inicializado");
}

LiquidCrystal_I2C& getLCD() {
  return lcd;
}

LCDMode lcdMode = LCDMode::NORMAL;
unsigned long lcdModeUntil = 0;
String lastLine1 = "";
String lastLine2 = "";
String lastLine3 = "";
String lastLine4 = "";

// Función auxiliar para actualizar solo si cambió
void updateLCDLine(int line, const String& text) {
  String* lastLine;
  switch(line) {
    case 0: lastLine = &lastLine1; break;
    case 1: lastLine = &lastLine2; break;
    case 2: lastLine = &lastLine3; break;
    case 3: lastLine = &lastLine4; break;
    default: return;
  }
  
  if (*lastLine != text) {
    lcd.setCursor(0, line);
    // Limpiar la línea completa (20 caracteres)
    lcd.print("                    ");
    lcd.setCursor(0, line);
    lcd.print(text);
    *lastLine = text;
  }
}

void actualizarLCD() {
  // Si hay un modo temporal activo, no sobrescribir
  if (lcdMode != LCDMode::NORMAL) {
    // Verificar si el modo temporal expiró
    if (lcdModeUntil > 0 && millis() >= lcdModeUntil) {
      lcdMode = LCDMode::NORMAL;
      lastLine1 = "";  // Forzar actualización
      lastLine2 = "";
      lastLine3 = "";
      lastLine4 = "";
    } else {
      return;  // Mantener el mensaje actual
    }
  }

  String line1 = "";
  String line2 = "";
  String line3 = "";
  String line4 = "";

  if (apMode) {
    line1 = "Modo AP Activo";
    line2 = "SSID: GatewayMQTT";
    line3 = "IP: 192.168.4.1";
    line4 = "Config: Web Portal";
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      line1 = "WiFi: Conectado";
      line2 = "MQTT: " + String(mqttConnected ? "Conectado" : "Esperando...");
      line3 = "LoRa: " + loraStatus;
      line4 = "Endpoints: " + String(numEndpointsActivos);
    } else {
      line1 = "WiFi: Desconectado";
      line2 = "Buscando red...";
      line3 = "";
      line4 = "";
    }
  }

  updateLCDLine(0, line1);
  updateLCDLine(1, line2);
  updateLCDLine(2, line3);
  updateLCDLine(3, line4);
}

void mostrarMensajeLCD(const String& linea1, const String& linea2, unsigned long duracion) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
  
  lastLine1 = linea1;
  lastLine2 = linea2;
  lastLine3 = "";
  lastLine4 = "";
  
  if (duracion > 0) {
    lcdModeUntil = millis() + duracion;
  } else {
    lcdModeUntil = 0;  // Permanente hasta cambio explícito
  }
}

void mostrarMensajeLCD(const String& linea1, const String& linea2, const String& linea3, const String& linea4, unsigned long duracion) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
  lcd.setCursor(0, 2);
  lcd.print(linea3);
  lcd.setCursor(0, 3);
  lcd.print(linea4);
  
  lastLine1 = linea1;
  lastLine2 = linea2;
  lastLine3 = linea3;
  lastLine4 = linea4;
  
  if (duracion > 0) {
    lcdModeUntil = millis() + duracion;
  } else {
    lcdModeUntil = 0;
  }
}

void volverModoNormal() {
  lcdMode = LCDMode::NORMAL;
  lcdModeUntil = 0;
  lastLine1 = "";
  lastLine2 = "";
  lastLine3 = "";
  lastLine4 = "";
  lcd.clear();
}