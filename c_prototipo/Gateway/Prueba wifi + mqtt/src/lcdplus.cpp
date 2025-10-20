// lcdplus.cpp
#include "lcdplus.h"
#include "conexiones.h"
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>

// Instancia del LCD (dirección I2C: 0x27 o 0x3F, 16 columnas, 2 filas)
static LiquidCrystal_I2C lcd(0x27, 16, 2);

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

// Función auxiliar para actualizar solo si cambió
void updateLCDLine(int line, const String& text) {
  String* lastLine = (line == 0) ? &lastLine1 : &lastLine2;
  
  if (*lastLine != text) {
    lcd.setCursor(0, line);
    // Limpiar la línea completa
    lcd.print("                ");
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
    } else {
      return;  // Mantener el mensaje actual
    }
  }

  String line1 = "";
  String line2 = "";

  if (apMode) {
    line1 = "Modo AP Activo";
    line2 = "IP: 192.168.4.1";
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      line1 = "WiFi: Conectado";
      line2 = "MQTT: " + String(mqttConnected ? "Conectado" : "Esperando...");
    } else {
      line1 = "WiFi: Buscando";
      line2 = "Red...";
    }
  }

  updateLCDLine(0, line1);
  updateLCDLine(1, line2);
}

void mostrarMensajeLCD(const String& linea1, const String& linea2, unsigned long duracion) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(linea1);
  lcd.setCursor(0, 1);
  lcd.print(linea2);
  
  lastLine1 = linea1;
  lastLine2 = linea2;
  
  if (duracion > 0) {
    lcdModeUntil = millis() + duracion;
  } else {
    lcdModeUntil = 0;  // Permanente hasta cambio explícito
  }
}

void volverModoNormal() {
  lcdMode = LCDMode::NORMAL;
  lcdModeUntil = 0;
  lastLine1 = "";
  lastLine2 = "";
  lcd.clear();
}