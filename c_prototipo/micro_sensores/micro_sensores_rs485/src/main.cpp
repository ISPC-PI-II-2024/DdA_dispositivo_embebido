#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <SoftwareSerial.h>

// === CONFIGURACIÓN DEL DISPOSITIVO ===
#define MY_ADDRESS 0x01  //  DIRECCIÓN ÚNICA - Cambiar por cada ESP8266
#define DEVICE_TYPE "AHT10_SENSOR"
#define DEVICE_LOCATION "Silo_Norte"

// === CONFIGURACIÓN DE PINES ESP8266 ===
#define SDA_PIN 4     // GPIO4 (D2)
#define SCL_PIN 5     // GPIO5 (D1) 
#define RX_PIN 13     // GPIO13 (D7) - RO del MAX485
#define TX_PIN 12     // GPIO12 (D6) - DI del MAX485
#define DE_RE_PIN 14  // GPIO14 (D5) - DE y RE juntos

// === INTERVALOS DE TIEMPO ===
#define HEARTBEAT_INTERVAL 30000    // 30 segundos
#define SENSOR_READ_INTERVAL 5000   // 5 segundos (lectura local)

Adafruit_AHTX0 aht;
SoftwareSerial RS485(RX_PIN, TX_PIN);

// === VARIABLES GLOBALES ===
unsigned long lastHeartbeat = 0;
unsigned long lastSensorRead = 0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;
bool sensorActivo = false;
String deviceStatus = "BOOTING";

// === FUNCIÓN PARA CALCULAR CRC SIMPLE ===
String calcularCRC(String mensaje) {
  uint16_t crc = 0;
  for (size_t i = 0; i < mensaje.length(); i++) {
    crc += mensaje[i];
  }
  return String(crc, HEX);
}

// === FUNCIÓN ENVIAR POR RS485 ===
void enviarRS485(String mensaje) {
  // Agregar CRC al mensaje
  String mensajeConCRC = mensaje + "|CRC:" + calcularCRC(mensaje);
  
  digitalWrite(DE_RE_PIN, HIGH);  // Modo transmisión
  RS485.println(mensajeConCRC);
  RS485.flush();
  delay(10); // Pequeña pausa para estabilidad
  digitalWrite(DE_RE_PIN, LOW);   // Modo recepción
  
  Serial.println("📤 RS485 >>> " + mensajeConCRC);
}

// === FUNCIÓN LEER SENSOR ===
bool leerSensor() {
  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    lastTemperature = temp.temperature;
    lastHumidity = humidity.relative_humidity;
    return true;
  }
  return false;
}

// === FUNCIÓN PROCESAR COMANDOS RS485 ===
void procesarComando(String comando) {
  Serial.println("📥 Comando recibido: " + comando);
  
  // Verificar CRC (opcional para esta versión)
  
  //  COMANDO: DISCOVERY - Identificación del dispositivo
  if (comando.indexOf("CMD:DISCOVERY") != -1) {
    String respuesta = "ADDR:" + String(MY_ADDRESS, HEX) + "|";
    respuesta += "CMD:DISCOVERY_RESP|";
    respuesta += "TYPE:" + String(DEVICE_TYPE) + "|";
    respuesta += "LOC:" + String(DEVICE_LOCATION) + "|";
    respuesta += "STATUS:" + deviceStatus + "|";
    respuesta += "TEMP:" + String(lastTemperature, 1) + "|";
    respuesta += "HUM:" + String(lastHumidity, 1);
    
    enviarRS485(respuesta);
  }
  
  //  COMANDO: READ_SENSOR - Lectura inmediata
  else if (comando.indexOf("CMD:READ_SENSOR") != -1) {
    if (leerSensor()) {
      String respuesta = "ADDR:" + String(MY_ADDRESS, HEX) + "|";
      respuesta += "CMD:SENSOR_DATA|";
      respuesta += "TEMP:" + String(lastTemperature, 1) + "|";
      respuesta += "HUM:" + String(lastHumidity, 1) + "|";
      respuesta += "UNIT_TEMP:C|UNIT_HUM:%|";
      respuesta += "TIMESTAMP:" + String(millis());
      
      enviarRS485(respuesta);
    } else {
      enviarRS485("ADDR:" + String(MY_ADDRESS, HEX) + "|CMD:ERROR|MSG:SENSOR_FAIL");
    }
  }
  
  //  COMANDO: CONFIG - Configuración parámetros
  else if (comando.indexOf("CMD:CONFIG") != -1) {
    // Ejemplo: procesar configuración de intervalos
    enviarRS485("ADDR:" + String(MY_ADDRESS, HEX) + "|CMD:CONFIG_ACK|STATUS:OK");
  }
  
  //  COMANDO: STATUS - Estado del dispositivo
  else if (comando.indexOf("CMD:STATUS") != -1) {
    String respuesta = "ADDR:" + String(MY_ADDRESS, HEX) + "|";
    respuesta += "CMD:STATUS_RESP|";
    respuesta += "SENSOR_ACTIVE:" + String(sensorActivo ? "YES" : "NO") + "|";
    respuesta += "LAST_READ:" + String(lastSensorRead) + "|";
    respuesta += "UPTIME:" + String(millis() / 1000) + "s|";
    respuesta += "RSSI:" + String(WiFi.RSSI()) + "dBm";
    
    enviarRS485(respuesta);
  }
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("🚀 MICRO-SENSOR RS485 - INICIANDO");
  Serial.println("=========================================");
  Serial.println("📍 Dirección: " + String(MY_ADDRESS, HEX));
  Serial.println("📡 Tipo: " + String(DEVICE_TYPE));
  Serial.println("🏠 Ubicación: " + String(DEVICE_LOCATION));
  
  // Inicializar I2C y sensor AHT10
  Serial.print("🔍 Inicializando sensor AHT10... ");
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (aht.begin()) {
    sensorActivo = true;
    deviceStatus = "READY";
    Serial.println("✅ OK");
    
    // Lectura inicial del sensor
    if (leerSensor()) {
      Serial.println("🌡️  Lectura inicial - Temp: " + String(lastTemperature, 1) + "°C, Hum: " + String(lastHumidity, 1) + "%");
    }
  } else {
    sensorActivo = false;
    deviceStatus = "SENSOR_ERROR";
    Serial.println("❌ FALLO");
  }
  
  // Inicializar RS485
  Serial.print("🔌 Inicializando RS485... ");
  RS485.begin(9600);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW);
  Serial.println("✅ OK (9600 baud)");
  
  Serial.println();
  Serial.println("🎯 COMANDOS DISPONIBLES:");
  Serial.println("   - DISCOVERY: Identificación del dispositivo");
  Serial.println("   - READ_SENSOR: Lectura inmediata de sensores");
  Serial.println("   - STATUS: Estado del dispositivo");
  Serial.println("   - CONFIG: Configuración de parámetros");
  Serial.println();
  Serial.println("✅ SISTEMA INICIADO - Esperando comandos...");
  Serial.println("=========================================");
}

// === LOOP PRINCIPAL ===
void loop() {
  unsigned long ahora = millis();
  
  //  ESCUCHAR COMANDOS RS485 DEL ESP32-C3
  if (RS485.available()) {
    String comando = RS485.readString();
    comando.trim();
    
    // Verificar si el comando es para esta dirección
    String addrPattern = "ADDR:" + String(MY_ADDRESS, HEX);
    if (comando.indexOf(addrPattern) != -1 || comando.indexOf("ADDR:BROADCAST") != -1) {
      procesarComando(comando);
    }
  }
  
  //  HEARTBEAT AUTOMÁTICO CADA 30s
  if (ahora - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = ahora;
    
    String heartbeat = "ADDR:" + String(MY_ADDRESS, HEX) + "|";
    heartbeat += "CMD:HEARTBEAT|";
    heartbeat += "STATUS:" + deviceStatus + "|";
    heartbeat += "UPTIME:" + String(ahora / 1000) + "s";
    
    enviarRS485(heartbeat);
    Serial.println("💓 Heartbeat enviado");
  }
  
  //  LECTURA PERIÓDICA DEL SENSOR (solo para monitor local)
  if (sensorActivo && (ahora - lastSensorRead >= SENSOR_READ_INTERVAL)) {
    lastSensorRead = ahora;
    if (leerSensor()) {
      Serial.println("📊 Sensor - Temp: " + String(lastTemperature, 1) + "°C, Hum: " + String(lastHumidity, 1) + "%");
    }
  }
  
  delay(100); // Pequeño delay para estabilidad
}