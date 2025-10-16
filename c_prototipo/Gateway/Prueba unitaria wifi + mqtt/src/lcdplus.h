// lcdplus.h
#ifndef LCDPLUS_H
#define LCDPLUS_H

#include <Arduino.h>

// Forward declaration
class LiquidCrystal_I2C;

void initLCD();
void actualizarLCD();
LiquidCrystal_I2C& getLCD();

#endif