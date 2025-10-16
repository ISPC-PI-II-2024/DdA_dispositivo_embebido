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

void actualizarLCD() {
  lcd.clear();
  
  if (apMode) {
    lcd.setCursor(0, 0);
    lcd.print("Modo AP activo");
    lcd.setCursor(0, 1);
    lcd.print("192.168.4.1");
  } else if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi: OK");
    lcd.setCursor(0, 1);
    lcd.print("MQTT: ");
    lcd.print(mqttConnected ? "OK" : "NO");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi: Descon.");
    lcd.setCursor(0, 1);
    lcd.print("Reintentando...");
  }
}