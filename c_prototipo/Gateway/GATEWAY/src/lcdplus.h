// lcdplus.h
#ifndef LCDPLUS_H
#define LCDPLUS_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

void initLCD();
void actualizarLCD();
LiquidCrystal_I2C& getLCD();
void mostrarMensajeLCD(const String& linea1, const String& linea2, unsigned long duracion = 0);
void mostrarMensajeLCD(const String& linea1, const String& linea2, const String& linea3, const String& linea4, unsigned long duracion = 0);
void volverModoNormal();

enum class LCDMode {
  NORMAL,
  RESET_PROMPT,
  RESET_PROGRESS,
  RESET_CONFIRM,
  RESET_CANCEL
};

extern LCDMode lcdMode;
extern unsigned long lcdModeUntil;

#endif