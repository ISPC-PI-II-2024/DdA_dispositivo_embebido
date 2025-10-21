#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// === CONFIGURACIÓN RS485 (MAESTRO) ===
#define RS485_RX_PIN 4    // GPIO4 - RX del UART1
#define RS485_TX_PIN 5    // GPIO5 - TX del UART1  
#define RS485_DE_RE_PIN 6 // GPIO6 - Control DE/RE
HardwareSerial RS485(1);  // UART1

// === CONFIGURACIÓN LoRa PARA ESP32-C3 ===
#define LORA_SCK 2
#define LORA_MISO 3
#define LORA_MOSI 4
#define LORA_CS 7
#define LORA_RST 8
#define LORA_DIO0 9
#define LORA_FREQ 433E6

// === CONFIGURACIÓN SISTEMA ===
#define ENDPOINT_ID "EP01"
#define CICLO_COMPLETO 120000    // 2 minutos para pruebas
#define DESCUBRIMIENTO 10000     // 10 segundos para pruebas
#define LECTURA_SENSORES 30000   // 30 segundos para pruebas
#define ENVIO_LORA 15000         // 15 segundos para pruebas

// === VARIABLES GLOBALES ===
unsigned long cicloInicio = 0;
unsigned long ultimoEnvioLoRa = 0;
bool cicloEnProgreso = false;
int dispositivosActivos = 0;

// Estructura para almacenar datos de sensores
struct SensorData {
  String address;
  String type;
  String location;
  float temperature;
  float humidity;
  String status;
  unsigned long timestamp;
};

SensorData sensores[32];
int totalSensores = 0;

// === FUNCIÓN AUXILIAR EXTRAER VALORES (FALTABA ESTA FUNCIÓN) ===
String extraerValor(String texto, String clave) {
  int startIndex = texto.indexOf(clave);
  if (startIndex == -1) return "";
  
  startIndex += clave.length();
  int endIndex = texto.indexOf('|', startIndex);
  if (endIndex == -1) endIndex = texto.length();
  
  return texto.substring(startIndex, endIndex);
}

// === FUNCIÓN INICIALIZAR LoRa ===
bool iniciarLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("❌ Error iniciando LoRa!");
    return false;
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println("✅ LoRa inicializado - SF7, BW125, 433MHz");
  return true;
}

// === FUNCIÓN ENVIAR COMANDO RS485 ===
void enviarComandoRS485(String direccion, String comando) {
  String mensaje = "ADDR:" + direccion + "|CMD:" + comando;
  
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  RS485.println(mensaje);
  RS485.flush();
  delay(50);
  digitalWrite(RS485_DE_RE_PIN, LOW);
  
  Serial.println("📤 RS485 >>> " + mensaje);
}

// === FUNCIÓN BROADCAST RS485 ===
void broadcastRS485(String comando) {
  String mensaje = "ADDR:BROADCAST|CMD:" + comando;
  
  digitalWrite(RS485_DE_RE_PIN, HIGH);
  RS485.println(mensaje);
  RS485.flush();
  delay(50);
  digitalWrite(RS485_DE_RE_PIN, LOW);
  
  Serial.println("📤 RS485 BROADCAST >>> " + mensaje);
}

// === FUNCIÓN PROCESAR RESPUESTA RS485 ===
void procesarRespuestaRS485(String respuesta) {
  Serial.println("📥 RS485 <<< " + respuesta);
  
  int addrIndex = respuesta.indexOf("ADDR:");
  if (addrIndex == -1) return;
  
  int pipeIndex = respuesta.indexOf('|', addrIndex);
  String direccion = respuesta.substring(addrIndex + 5, pipeIndex);
  
  // 🎯 DISCOVERY RESPONSE
  if (respuesta.indexOf("CMD:DISCOVERY_RESP") != -1) {
    String tipo = extraerValor(respuesta, "TYPE:");
    String ubicacion = extraerValor(respuesta, "LOC:");
    String estado = extraerValor(respuesta, "STATUS:");
    String tempStr = extraerValor(respuesta, "TEMP:");
    String humStr = extraerValor(respuesta, "HUM:");
    
    bool sensorExistente = false;
    for (int i = 0; i < totalSensores; i++) {
      if (sensores[i].address == direccion) {
        sensores[i].type = tipo;
        sensores[i].location = ubicacion;
        sensores[i].status = estado;
        sensores[i].temperature = tempStr.toFloat();
        sensores[i].humidity = humStr.toFloat();
        sensores[i].timestamp = millis();
        sensorExistente = true;
        break;
      }
    }
    
    if (!sensorExistente && totalSensores < 32) {
      sensores[totalSensores].address = direccion;
      sensores[totalSensores].type = tipo;
      sensores[totalSensores].location = ubicacion;
      sensores[totalSensores].status = estado;
      sensores[totalSensores].temperature = tempStr.toFloat();
      sensores[totalSensores].humidity = humStr.toFloat();
      sensores[totalSensores].timestamp = millis();
      totalSensores++;
      dispositivosActivos++;
    }
    
    Serial.println("🔍 Sensor: " + direccion + " - " + ubicacion);
  }
  
  // 🎯 SENSOR DATA
  else if (respuesta.indexOf("CMD:SENSOR_DATA") != -1) {
    String tempStr = extraerValor(respuesta, "TEMP:");
    String humStr = extraerValor(respuesta, "HUM:");
    
    for (int i = 0; i < totalSensores; i++) {
      if (sensores[i].address == direccion) {
        sensores[i].temperature = tempStr.toFloat();
        sensores[i].humidity = humStr.toFloat();
        sensores[i].timestamp = millis();
        sensores[i].status = "ACTIVE";
        break;
      }
    }
    
    Serial.println("🌡️  " + direccion + " - Temp: " + tempStr + "°C, Hum: " + humStr + "%");
  }
}

// === FUNCIÓN ENVIAR POR LoRa (VERSIÓN ACTUALIZADA) ===
void enviarDatosLoRa() {
  JsonDocument doc;  // Usar JsonDocument en lugar de DynamicJsonDocument
  
  doc["endpoint_id"] = ENDPOINT_ID;
  doc["timestamp"] = millis();
  doc["dispositivos_activos"] = dispositivosActivos;
  doc["total_sensores"] = totalSensores;
  
  JsonArray sensoresArray = doc["sensores"].to<JsonArray>();  // Sintaxis moderna
  
  for (int i = 0; i < totalSensores; i++) {
    JsonObject sensor = sensoresArray.add<JsonObject>();  // Sintaxis moderna
    sensor["address"] = sensores[i].address;
    sensor["type"] = sensores[i].type;
    sensor["location"] = sensores[i].location;
    sensor["temperature"] = sensores[i].temperature;
    sensor["humidity"] = sensores[i].humidity;
    sensor["status"] = sensores[i].status;
    sensor["last_update"] = sensores[i].timestamp;
  }
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  LoRa.beginPacket();
  LoRa.print(jsonString);
  LoRa.endPacket();
  
  Serial.println("📡 LoRa >>> " + jsonString);
  Serial.println("✅ Datos enviados - Sensores: " + String(totalSensores));
}

// === FUNCIÓN CICLO DISCOVERY ===
void ejecutarDiscovery() {
  Serial.println("🔍 INICIANDO DESCUBRIMIENTO...");
  broadcastRS485("DISCOVERY");
  
  unsigned long inicioDiscovery = millis();
  while (millis() - inicioDiscovery < DESCUBRIMIENTO) {
    if (RS485.available()) {
      String respuesta = RS485.readString();
      respuesta.trim();
      procesarRespuestaRS485(respuesta);
    }
    delay(100);
  }
  
  Serial.println("✅ Descubrimiento completado - Dispositivos: " + String(dispositivosActivos));
}

// === FUNCIÓN CICLO LECTURA SENSORES ===
void ejecutarLecturaSensores() {
  Serial.println("🌡️ INICIANDO LECTURA...");
  
  for (int i = 0; i < totalSensores; i++) {
    Serial.println("📖 Leyendo: " + sensores[i].address + " - " + sensores[i].location);
    enviarComandoRS485(sensores[i].address, "READ_SENSOR");
    delay(2000);
    
    unsigned long inicioLectura = millis();
    while (millis() - inicioLectura < 3000) {
      if (RS485.available()) {
        String respuesta = RS485.readString();
        respuesta.trim();
        procesarRespuestaRS485(respuesta);
        break;
      }
      delay(100);
    }
  }
  
  Serial.println("✅ Lectura completada");
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("🚀 ENDPOINT ESP32-C3 - INICIANDO");
  Serial.println("=========================================");
  Serial.println("📍 ID: " + String(ENDPOINT_ID));
  Serial.println("📡 Modo: RS485 Maestro → LoRa");
  
  // Inicializar RS485 en UART1
  Serial.print("🔌 Inicializando RS485... ");
  RS485.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  digitalWrite(RS485_DE_RE_PIN, LOW);
  Serial.println("✅ OK");
  
  // Inicializar LoRa
  Serial.print("📡 Inicializando LoRa... ");
  if (iniciarLoRa()) {
    Serial.println("✅ OK");
  } else {
    Serial.println("❌ FALLO - Verificar conexiones");
    while(1) delay(1000);
  }
  
  Serial.println();
  Serial.println("🎯 CICLO AUTOMÁTICO (2 minutos para pruebas):");
  Serial.println("   1. Discovery (10s)");
  Serial.println("   2. Lectura (30s)"); 
  Serial.println("   3. Envío LoRa (15s)");
  Serial.println("   4. Espera (65s)");
  Serial.println();
  Serial.println("✅ SISTEMA INICIADO");
  Serial.println("=========================================");
  
  cicloInicio = millis();
  cicloEnProgreso = true;
}

// === LOOP PRINCIPAL ===
void loop() {
  unsigned long ahora = millis();
  unsigned long tiempoCiclo = ahora - cicloInicio;
  
  if (cicloEnProgreso) {
    if (tiempoCiclo < DESCUBRIMIENTO) {
      if (tiempoCiclo < 2000) {
        ejecutarDiscovery();
      }
    }
    else if (tiempoCiclo < (DESCUBRIMIENTO + LECTURA_SENSORES)) {
      if (tiempoCiclo < (DESCUBRIMIENTO + 2000)) {
        ejecutarLecturaSensores();
      }
    }
    else if (tiempoCiclo < (DESCUBRIMIENTO + LECTURA_SENSORES + ENVIO_LORA)) {
      if (tiempoCiclo < (DESCUBRIMIENTO + LECTURA_SENSORES + 2000)) {
        Serial.println("📡 ENVIANDO POR LoRa...");
        enviarDatosLoRa();
        ultimoEnvioLoRa = ahora;
      }
    }
    else {
      Serial.println("💤 CICLO COMPLETADO");
      Serial.println("   Dispositivos: " + String(dispositivosActivos));
      cicloEnProgreso = false;
    }
  }
  
  if (tiempoCiclo >= CICLO_COMPLETO) {
    cicloInicio = ahora;
    cicloEnProgreso = true;
    Serial.println("🔄 NUEVO CICLO...");
  }
  
  if (RS485.available()) {
    String respuesta = RS485.readString();
    respuesta.trim();
    procesarRespuestaRS485(respuesta);
  }
  
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String ack = "";
    while (LoRa.available()) {
      ack += (char)LoRa.read();
    }
    Serial.println("📥 LoRa ACK: " + ack);
  }
  
  delay(100);
}