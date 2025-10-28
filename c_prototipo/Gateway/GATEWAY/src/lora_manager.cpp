// lora_manager.cpp
#include "lora_manager.h"
#include "conexiones.h"

// Variables globales
EndpointInfo endpoints[MAX_ENDPOINTS];
int numEndpointsActivos = 0;
bool loraInicializado = false;
String loraStatus = "offline";

// Inicializar módulo LoRa
bool initLora() {
  Serial.println("🔧 Inicializando LoRa...");
  
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("❌ LoRa: Error al inicializar");
    loraStatus = "error";
    return false;
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  
  Serial.println("✅ LoRa inicializado correctamente");
  Serial.printf("   Frecuencia: %.0f MHz\n", LORA_FREQ / 1E6);
  
  // Inicializar estructura de endpoints
  for (int i = 0; i < MAX_ENDPOINTS; i++) {
    endpoints[i].activo = false;
    endpoints[i].numSensores = 0;
  }
  
  loraInicializado = true;
  loraStatus = "ok";
  return true;
}

// Enviar comando LoRa y esperar respuesta
String enviarComandoLora(const String& comando, unsigned long timeout = LORA_TIMEOUT) {
  // Limpiar buffer de recepción
  while (LoRa.available()) {
    LoRa.read();
  }
  
  // Enviar comando
  LoRa.beginPacket();
  LoRa.print(comando);
  LoRa.endPacket();
  
  Serial.println("📤 LoRa TX: " + comando);
  
  // Esperar respuesta
  unsigned long inicio = millis();
  String respuesta = "";
  
  while (millis() - inicio < timeout) {
    int packetSize = LoRa.parsePacket();
    
    if (packetSize > 0) {
      while (LoRa.available()) {
        respuesta += (char)LoRa.read();
      }
      
      int rssi = LoRa.packetRssi();
      float snr = LoRa.packetSnr();
      
      Serial.println("📥 LoRa RX: " + respuesta);
      Serial.printf("   RSSI: %d dBm, SNR: %.2f dB\n", rssi, snr);
      
      return respuesta;
    }
    
    delay(10);
  }
  
  Serial.println("⚠️ LoRa: Timeout esperando respuesta");
  return "";
}

// Solicitar lista de endpoints disponibles
// Comando: "LIST_ENDPOINTS"
// Respuesta: "ENDPOINTS:E01,E02,E03"
bool solicitarListaEndpoints() {
  Serial.println("\n🔍 Solicitando lista de endpoints...");
  
  String respuesta = enviarComandoLora("LIST_ENDPOINTS");
  
  if (respuesta.length() == 0) {
    Serial.println("❌ Sin respuesta de endpoints");
    numEndpointsActivos = 0;
    return false;
  }
  
  // Parsear respuesta
  if (!respuesta.startsWith("ENDPOINTS:")) {
    Serial.println("⚠️ Formato de respuesta inválido");
    return false;
  }
  
  String listaIds = respuesta.substring(10);
  listaIds.trim();
  
  // Limpiar endpoints anteriores
  numEndpointsActivos = 0;
  
  // Parsear IDs separados por coma
  int idx = 0;
  int lastIdx = 0;
  
  while (idx < listaIds.length() && numEndpointsActivos < MAX_ENDPOINTS) {
    idx = listaIds.indexOf(',', lastIdx);
    
    String endpointId;
    if (idx == -1) {
      endpointId = listaIds.substring(lastIdx);
      idx = listaIds.length();
    } else {
      endpointId = listaIds.substring(lastIdx, idx);
    }
    
    endpointId.trim();
    
    if (endpointId.length() > 0) {
      endpoints[numEndpointsActivos].id = endpointId;
      endpoints[numEndpointsActivos].activo = true;
      numEndpointsActivos++;
      Serial.println("  ✓ Endpoint detectado: " + endpointId);
    }
    
    lastIdx = idx + 1;
  }
  
  Serial.printf("✅ Total endpoints activos: %d\n", numEndpointsActivos);
  return numEndpointsActivos > 0;
}

// Solicitar datos de un endpoint específico
// Comando: "GET_DATA:E01"
// Respuesta: "DATA:E01|BAT:99|CHG:1|SNS:2|S1:0F01,1,17.7,62,ok|S2:0F2A,2,20.8,63,ok"
bool solicitarDatosEndpoint(const String& endpointId) {
  Serial.println("\n📊 Solicitando datos de endpoint: " + endpointId);
  
  String comando = "GET_DATA:" + endpointId;
  String respuesta = enviarComandoLora(comando);
  
  if (respuesta.length() == 0) {
    Serial.println("❌ Sin respuesta del endpoint " + endpointId);
    
    // Marcar endpoint como inactivo
    for (int i = 0; i < numEndpointsActivos; i++) {
      if (endpoints[i].id == endpointId) {
        endpoints[i].loraStatus = "timeout";
        break;
      }
    }
    return false;
  }
  
  // Parsear respuesta: "DATA:E01|BAT:99|CHG:1|SNS:2|S1:0F01,1,17.7,62,ok|S2:0F2A,2,20.8,63,ok"
  if (!respuesta.startsWith("DATA:")) {
    Serial.println("⚠️ Formato de respuesta inválido");
    return false;
  }
  
  // Buscar índice del endpoint
  int epIdx = -1;
  for (int i = 0; i < numEndpointsActivos; i++) {
    if (endpoints[i].id == endpointId) {
      epIdx = i;
      break;
    }
  }
  
  if (epIdx == -1) {
    Serial.println("⚠️ Endpoint no encontrado en lista");
    return false;
  }
  
  // Parsear campos separados por |
  int currentPos = 0;
  int nextPipe = 0;
  
  while (nextPipe < respuesta.length()) {
    nextPipe = respuesta.indexOf('|', currentPos);
    if (nextPipe == -1) nextPipe = respuesta.length();
    
    String campo = respuesta.substring(currentPos, nextPipe);
    
    if (campo.startsWith("BAT:")) {
      endpoints[epIdx].bateria = campo.substring(4).toInt();
    }
    else if (campo.startsWith("CHG:")) {
      endpoints[epIdx].cargando = (campo.substring(4).toInt() == 1);
    }
    else if (campo.startsWith("SNS:")) {
      endpoints[epIdx].numSensores = campo.substring(4).toInt();
    }
    else if (campo.startsWith("S")) {
      // Sensor: "S1:0F01,1,17.7,62,ok"
      int colonIdx = campo.indexOf(':');
      if (colonIdx != -1) {
        int sensorIdx = campo.substring(1, colonIdx).toInt() - 1;
        
        if (sensorIdx >= 0 && sensorIdx < MAX_SENSORES_POR_ENDPOINT) {
          String sensorData = campo.substring(colonIdx + 1);
          
          // Parsear: "0F01,1,17.7,62,ok"
          int comma1 = sensorData.indexOf(',');
          int comma2 = sensorData.indexOf(',', comma1 + 1);
          int comma3 = sensorData.indexOf(',', comma2 + 1);
          int comma4 = sensorData.indexOf(',', comma3 + 1);
          
          if (comma1 != -1 && comma2 != -1 && comma3 != -1 && comma4 != -1) {
            endpoints[epIdx].sensores[sensorIdx].id = sensorData.substring(0, comma1);
            endpoints[epIdx].sensores[sensorIdx].posicion = sensorData.substring(comma1 + 1, comma2).toInt();
            endpoints[epIdx].sensores[sensorIdx].temperatura = sensorData.substring(comma2 + 1, comma3).toFloat();
            endpoints[epIdx].sensores[sensorIdx].humedad = sensorData.substring(comma3 + 1, comma4).toFloat();
            endpoints[epIdx].sensores[sensorIdx].estado = sensorData.substring(comma4 + 1);
            endpoints[epIdx].sensores[sensorIdx].valido = true;
          }
        }
      }
    }
    
    currentPos = nextPipe + 1;
  }
  
  // Obtener RSSI y SNR del último paquete
  endpoints[epIdx].rssi = LoRa.packetRssi();
  endpoints[epIdx].snr = LoRa.packetSnr();
  endpoints[epIdx].loraStatus = "ok";
  endpoints[epIdx].ultimaActualizacion = millis();
  
  Serial.printf("✅ Endpoint %s: BAT=%d%%, CHG=%s, Sensores=%d\n",
    endpointId.c_str(),
    endpoints[epIdx].bateria,
    endpoints[epIdx].cargando ? "SI" : "NO",
    endpoints[epIdx].numSensores
  );
  
  return true;
}

// Actualizar todos los endpoints (polling secuencial)
void actualizarTodosEndpoints() {
  if (!loraInicializado) return;
  
  Serial.println("\n🔄 Iniciando actualización de endpoints...");
  
  // Paso 1: Solicitar lista de endpoints
  if (!solicitarListaEndpoints()) {
    Serial.println("⚠️ No se pudo obtener lista de endpoints");
    return;
  }
  
  delay(200);  // Pequeña pausa entre comandos
  
  // Paso 2: Solicitar datos de cada endpoint
  for (int i = 0; i < numEndpointsActivos; i++) {
    solicitarDatosEndpoint(endpoints[i].id);
    delay(200);  // Pausa entre solicitudes
  }
  
  Serial.println("✅ Actualización de endpoints completada\n");
}

// Loop LoRa - escuchar mensajes no solicitados (opcional)
void loraLoop() {
  if (!loraInicializado) return;
  
  // Este loop puede usarse para recibir notificaciones push de endpoints
  // Por ahora, el sistema funciona con polling
}

// Generar JSON para tópico gateway/gateway
String getGatewayStatusJSON() {
  DynamicJsonDocument doc(512);
  
  doc["id_gateway"] = gatewayId;
  doc["wifi_signal"] = calidadSenalWifi(WiFi.RSSI());
  doc["lora_status"] = loraStatus;
  
  // Calcular uptime
  unsigned long uptimeMs = millis();
  unsigned long hours = uptimeMs / 3600000;
  unsigned long minutes = (uptimeMs % 3600000) / 60000;
  unsigned long seconds = (uptimeMs % 60000) / 1000;
  
  char uptimeStr[12];
  sprintf(uptimeStr, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  doc["uptime"] = uptimeStr;
  
  String output;
  serializeJson(doc, output);
  return output;
}

// Generar JSON para tópico gateway/endpoint
String getEndpointsStatusJSON() {
  DynamicJsonDocument doc(2048);
  
  doc["id_gateway"] = gatewayId;
  
  JsonArray endpointsArray = doc.createNestedArray("endpoints");
  
  for (int i = 0; i < numEndpointsActivos; i++) {
    if (!endpoints[i].activo) continue;
    
    JsonObject ep = endpointsArray.createNestedObject();
    ep["id"] = endpoints[i].id;
    ep["bateria"] = endpoints[i].bateria;
    ep["cargando"] = endpoints[i].cargando;
    ep["lora"] = endpoints[i].loraStatus;
    ep["sensores"] = endpoints[i].numSensores;
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

// Generar JSON para tópico gateway/sensor
String getSensorsDataJSON() {
  DynamicJsonDocument doc(4096);
  
  doc["id_gateway"] = gatewayId;
  
  JsonArray endpointsArray = doc.createNestedArray("endpoints");
  
  for (int i = 0; i < numEndpointsActivos; i++) {
    if (!endpoints[i].activo) continue;
    
    JsonObject ep = endpointsArray.createNestedObject();
    ep["id_endpoint"] = endpoints[i].id;
    
    JsonArray sensoresArray = ep.createNestedArray("sensores");
    
    for (int j = 0; j < endpoints[i].numSensores; j++) {
      if (endpoints[i].sensores[j].valido) {
        JsonObject sensor = sensoresArray.createNestedObject();
        sensor["id"] = endpoints[i].sensores[j].id;
        sensor["posicion"] = endpoints[i].sensores[j].posicion;
        sensor["temp"] = endpoints[i].sensores[j].temperatura;
        sensor["humedad"] = endpoints[i].sensores[j].humedad;
        sensor["estado"] = endpoints[i].sensores[j].estado;
      }
    }
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

// Limpiar endpoints inactivos
void limpiarEndpointsInactivos() {
  unsigned long ahora = millis();
  
  for (int i = 0; i < numEndpointsActivos; i++) {
    if (endpoints[i].activo) {
      if (ahora - endpoints[i].ultimaActualizacion > 120000) {  // 2 minutos
        endpoints[i].activo = false;
        Serial.println("⚠️ Endpoint " + endpoints[i].id + " marcado como inactivo");
      }
    }
  }
}