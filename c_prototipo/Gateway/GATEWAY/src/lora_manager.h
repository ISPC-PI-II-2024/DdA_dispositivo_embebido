// lora_manager.h
#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// --- Pines LoRa RA-02 (Gateway ESP32) ---
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST 26
#define LORA_DIO0 27

// --- Frecuencia LoRa ---
#define LORA_FREQ 433E6

// --- Configuración de endpoints ---
#define MAX_ENDPOINTS 10
#define MAX_SENSORES_POR_ENDPOINT 10
#define LORA_TIMEOUT 5000  // 5 segundos timeout para respuestas

// Estructura para un sensor
struct SensorInfo {
  String id;          // ID del sensor (ej: "0F01")
  uint8_t posicion;   // Posición física del sensor
  float temperatura;
  float humedad;
  String estado;      // "ok", "error", "offline"
  bool valido;
};

// Estructura para un endpoint
struct EndpointInfo {
  String id;              // ID del endpoint (ej: "E01")
  uint8_t bateria;        // Porcentaje de batería (0-100)
  bool cargando;          // true si está cargando
  String loraStatus;      // "ok", "error"
  uint8_t numSensores;    // Cantidad de sensores conectados
  SensorInfo sensores[MAX_SENSORES_POR_ENDPOINT];
  int rssi;               // Señal LoRa
  float snr;              // Signal-to-Noise Ratio
  unsigned long ultimaActualizacion;
  bool activo;
};

// Variables globales
extern EndpointInfo endpoints[MAX_ENDPOINTS];
extern int numEndpointsActivos;
extern bool loraInicializado;
extern String loraStatus;  // "ok", "error", "offline"

// Funciones públicas
bool initLora();
void loraLoop();

// Polling de endpoints
bool solicitarListaEndpoints();
bool solicitarDatosEndpoint(const String& endpointId);
void actualizarTodosEndpoints();

// Generación de JSON para MQTT
String getGatewayStatusJSON();
String getEndpointsStatusJSON();
String getSensorsDataJSON();

// Utilidades
void limpiarEndpointsInactivos();

#endif