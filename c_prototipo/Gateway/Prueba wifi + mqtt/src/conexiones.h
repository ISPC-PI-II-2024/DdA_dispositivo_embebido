// conexiones.h
#ifndef CONEXIONES_H
#define CONEXIONES_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <WebSocketsClient.h>
#include <PubSubClient.h>

// Forward declaration
class WebSocketClient;

// Variables globales exportadas
extern String ssid;
extern String password;
extern bool wifiConfigured;
extern bool apMode;
extern bool mqttConnected;
extern Preferences preferences;
extern WebSocketsClient webSocket;
extern WebSocketClient* wsClient;
extern PubSubClient mqttClient;

// Constantes
extern const char* gatewayId;

// Funciones
void initWiFi();
void conectarWifi();
void iniciarAP();
void handleWiFiReconnect();
void initMQTT();
void publishStatus();
void reconnectMQTT();
String obtenerHoraArgentina();
String calidadSenalWifi(int rssi);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

#endif