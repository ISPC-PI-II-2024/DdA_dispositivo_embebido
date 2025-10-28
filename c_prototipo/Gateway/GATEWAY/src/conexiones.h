// conexiones.h
#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include "mqtt_manual.h"

// Variables globales exportadas
extern String ssid;
extern String password;
extern bool wifiConfigured;
extern bool apMode;
extern bool mqttConnected;
extern Preferences preferences;
extern WebSocketsClient webSocket;
extern MQTTManual* mqttClient;

// Constantes
extern const char* gatewayId;

extern const int RESET_BUTTON_PIN;  // Cambiar a GPIO 33 en conexiones.cpp

// Funciones
void initWiFi();
void conectarWifi();
void iniciarAP();
void handleWiFiReconnect();
void initMQTT();
void mqttLoop();
void publishStatus();
void clearWiFiConfig();
void checkResetButton();
String obtenerHoraArgentina();
String calidadSenalWifi(int rssi);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

#endif