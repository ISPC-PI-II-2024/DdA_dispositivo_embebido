// Endpoint con RS485 - Responde a consultas del Gateway
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ⚠️ CAMBIAR EN CADA ENDPOINT (E01, E02, E03, etc.)
#define ENDPOINT_ID "E01"

// --------------------------------------------------------------------------------------
// PINOUT PARA ESP32-S (WROOM)
// --------------------------------------------------------------------------------------

// Pines LoRa (ESP32-S)
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST 26
#define LORA_DIO0 27

// --- Pines LEDs esp32s (Salidas seguras para LEDs) ---
#define LED_ROJO 32
#define LED_AMARILLO 33
#define LED_VERDE 25 // Usamos 25, ya que 34 es pin de SÓLO entrada

/*
// --- Pines LoRa RA-02 (ESP32-C3 ORIGINAL) ---
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_CS 7
#define LORA_RST -1
#define LORA_DIO0 3

// --- Pines LEDs (ESP32-C3 ORIGINAL) ---
#define LED_ROJO 8
#define LED_AMARILLO 9
#define LED_VERDE 10

// --- Pin batería (ESP32-C3 ORIGINAL) ---
#define PIN_BATERIA A0  // ADC para leer batería
#define PIN_CARGANDO 20 // GPIO que detecta carga

// --- Pines RS485 (ORIGINAL) ---
#define DE_RE 2 // Dirección RS485 (GPIO2)
#define DI 1    // TX hacia MAX485
#define RO 0    // RX desde MAX485
*/
// --------------------------------------------------------------------------------------

// Configuración LoRa
#define LORA_FREQ 433E6
#define NUM_SENSORES_MAX 5
#define RS485_TIMEOUT 1000
#define LORA_TIMEOUT 3000

bool loraConectado = false;

// Estructura para datos de sensor
struct SensorData {
  String id;          // ID del sensor (ej: "0F01")
  uint8_t posicion;
  float temperatura;
  float humedad;
  String estado;
  bool valido;
};

SensorData sensores[NUM_SENSORES_MAX];
int numSensoresActivos = 0;

/*
// Control RS485 - Original Comentado
void rs485_txMode() {
  digitalWrite(DE_RE, HIGH);
  delayMicroseconds(100);
}

void rs485_rxMode() {
  digitalWrite(DE_RE, LOW);
  delayMicroseconds(100);
}
*/

// ----------------------------------------------------
// 🎯 FUNCIONES DE SIMULACIÓN DE BATERÍA Y CARGA
// ----------------------------------------------------

// Leer nivel de batería (0-100%)
uint8_t leerBateria() {
  // ⚠️ SIMULACIÓN: Devuelve un valor fijo.
  return 85; 
}

// Verificar si está cargando
bool estaCargando() {
  // ⚠️ SIMULACIÓN: Devuelve FALSE (0).
  return false;
}

/*
// Leer nivel de batería - Original Comentado
uint8_t leerBateria() {
  int valorADC = analogRead(PIN_BATERIA);
  // ... lógica original ...
  uint8_t porcentaje = map(valorADC, 0, 4095, 0, 100);
  if (porcentaje > 100) porcentaje = 100;
  return porcentaje;
}

// Verificar si está cargando - Original Comentado
bool estaCargando() {
  return digitalRead(PIN_CARGANDO) == HIGH;
}
*/

// Verificar versión del chip LoRa
uint8_t readLoRaVersionSPI() {
  uint8_t version;
  digitalWrite(LORA_CS, LOW);
  SPI.transfer(0x42 & 0x7F);
  version = SPI.transfer(0x00);
  digitalWrite(LORA_CS, HIGH);
  return version;
}

// Inicializar módulo LoRa
bool iniciarLora() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  
  if (!LoRa.begin(LORA_FREQ)) return false;
  
  uint8_t version = readLoRaVersionSPI();
  if (version != 0x12 && version != 0x13) {
    LoRa.end();
    return false;
  }
  
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  
  Serial.println("✅ LoRa OK");
  return true;
}

// Reconectar LoRa
void reconnectLora() {
  loraConectado = false;
  digitalWrite(LED_VERDE, LOW);
  LoRa.end();
  delay(200);
  
  while (!loraConectado) {
    digitalWrite(LED_ROJO, HIGH);
    delay(100);
    digitalWrite(LED_ROJO, LOW);
    delay(200);
    
    if (iniciarLora()) {
      loraConectado = true;
      digitalWrite(LED_VERDE, HIGH);
    }
  }
}

// ----------------------------------------------------
// 🎯 FUNCIÓN DE SIMULACIÓN (REEMPLAZA leerSensorRS485)
// ----------------------------------------------------
bool leerSensorSimulado(uint8_t sensorId, SensorData &data) {
  if (sensorId < 1 || sensorId > NUM_SENSORES_MAX) {
    data.valido = false;
    return false;
  }
  
  data.posicion = sensorId;
  data.estado = "ok";
  data.valido = true;
  
  // 1. ID del sensor simulado
  data.id = "E" + String(ENDPOINT_ID) + "-S" + String(sensorId);
  
  // 2. Temperatura (16.0 a 21.0 grados)
  // Generamos un entero entre 160 y 210 y lo dividimos por 10
  float temp = (float)random(160, 211) / 10.0;
  data.temperatura = temp;
  
  // 3. Humedad (56 a 62 %)
  // Generamos un entero entre 56 y 62
  float hum = (float)random(56, 63);
  data.humedad = hum;
  
  Serial.printf("✅ Sensor %d SIMULADO: ID=%s T=%.1f°C H=%.0f%% Estado=%s\n",
    sensorId, data.id.c_str(), data.temperatura, data.humedad, data.estado.c_str());
    
  return true;
}

/*
// Leer sensor remoto vía RS485 - Original Comentado
bool leerSensorRS485(uint8_t sensorId, SensorData &data) {
  // Limpiar buffer
  while (Serial1.available()) Serial1.read();
  
  // Enviar comando
  rs485_txMode();
  // ... resto de la lógica RS485 ...
  
  // Parsear respuesta
  // ... resto de la lógica RS485 ...
  
  return true;
}
*/

void leerTodosSensores() {
  Serial.println("\n📡 Leyendo sensores SIMULADOS...");
  
  numSensoresActivos = 0;
  
  for (int i = 0; i < NUM_SENSORES_MAX; i++) {
    SensorData data;
    
    // Usamos la función SIMULADA
    if (leerSensorSimulado(i + 1, data)) { 
      sensores[i] = data;
      numSensoresActivos++;
    } else {
      sensores[i].valido = false;
      sensores[i].id = "----";
      sensores[i].posicion = i + 1;
      sensores[i].estado = "offline";
    }
    
    delay(50); 
  }
  
  Serial.printf("✅ Sensores activos: %d/%d\n", numSensoresActivos, NUM_SENSORES_MAX);
}

// Procesar comandos LoRa del Gateway
void procesarComandoLora(const String& comando) {
  Serial.println("📥 Comando recibido: " + comando);
  
  digitalWrite(LED_AMARILLO, HIGH);
  
  String respuesta = "";
  
  // Comando 1: LIST_ENDPOINTS
  if (comando == "LIST_ENDPOINTS") {
    respuesta = "ENDPOINTS:" + String(ENDPOINT_ID);
    Serial.println("📤 Respondiendo a LIST_ENDPOINTS");
  }
  
  // Comando 2: GET_DATA:E01
  else if (comando.startsWith("GET_DATA:")) {
    String idSolicitado = comando.substring(9);
    idSolicitado.trim();
    
    if (idSolicitado == ENDPOINT_ID) {
      Serial.println("📤 Generando datos para " + idSolicitado);
      
      // Leer todos los sensores (Simulados)
      leerTodosSensores();
      
      // Construir respuesta
      // Formato: "DATA:E01|BAT:85|CHG:0|SNS:5|S1:E01-S1,1,18.5,58,ok|..."
      respuesta = "DATA:" + String(ENDPOINT_ID);
      respuesta += "|BAT:" + String(leerBateria()); // SIMULADA
      respuesta += "|CHG:" + String(estaCargando() ? 1 : 0); // SIMULADA
      respuesta += "|SNS:" + String(numSensoresActivos);
      
      // Agregar datos de cada sensor
      for (int i = 0; i < NUM_SENSORES_MAX; i++) {
        if (sensores[i].valido) {
          respuesta += "|S" + String(i + 1) + ":";
          respuesta += sensores[i].id + ",";
          respuesta += String(sensores[i].posicion) + ",";
          // Temperatura con un decimal, humedad sin decimales
          respuesta += String(sensores[i].temperatura, 1) + ",";
          respuesta += String(sensores[i].humedad, 0) + ","; 
          respuesta += sensores[i].estado;
        }
      }
    } else {
      Serial.println("⚠️ ID no coincide, ignorando");
      digitalWrite(LED_AMARILLO, LOW);
      return;
    }
  }
  
  // Enviar respuesta si se generó alguna
  if (respuesta.length() > 0) {
    delay(random(50, 150));
    
    LoRa.beginPacket();
    LoRa.print(respuesta);
    LoRa.endPacket();
    
    Serial.println("📤 LoRa TX: " + respuesta);
    
    digitalWrite(LED_ROJO, HIGH);
    delay(200);
    digitalWrite(LED_ROJO, LOW);
  }
  
  digitalWrite(LED_AMARILLO, LOW);
}

void setup() {
  Serial.begin(9600);
  // Inicializar la semilla aleatoria usando un pin analógico
  randomSeed(analogRead(LORA_DIO0)); 
  delay(500);
  
  Serial.println("\n=== Endpoint LoRa + RS485 (MODO SIMULACIÓN) ===");
  Serial.println("ID: " + String(ENDPOINT_ID));
  Serial.println("================================\n");
  
  // Configurar LEDs (Pines 32, 33, 25)
  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);
  
  // Secuencia de inicio (todos los LEDs)
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_AMARILLO, HIGH);
    digitalWrite(LED_VERDE, HIGH);
    delay(200);
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_AMARILLO, LOW);
    digitalWrite(LED_VERDE, LOW);
    delay(200);
  }
  
  // Inicializar RS485 (Comentado)
  /*
  Serial.println("🔧 Inicializando RS485...");
  pinMode(DE_RE, OUTPUT);
  rs485_rxMode();
  Serial1.begin(9600, SERIAL_8N1, DI, RO);
  Serial.println("✅ RS485 OK (9600 baud)");
  */
  
  delay(1000);
  
  // Inicializar LoRa
  Serial.println("🔧 Inicializando LoRa...");
  loraConectado = iniciarLora();
  
  if (!loraConectado) {
    Serial.println("❌ LoRa no inicializado, reintentando...");
    reconnectLora();
  } else {
    digitalWrite(LED_VERDE, HIGH);
  }
  
  Serial.println("\n✅ Endpoint listo - Esperando comandos del Gateway...\n");
  
  // Lectura inicial de sensores (Simulados)
  delay(2000);
  leerTodosSensores();
}

void loop() {
  // Verificar conexión LoRa
  if (!loraConectado) {
    reconnectLora();
    return;
  }
  
  // Escuchar comandos LoRa
  int packetSize = LoRa.parsePacket();
  
  if (packetSize > 0) {
    String comando = "";
    
    while (LoRa.available()) {
      comando += (char)LoRa.read();
    }
    
    comando.trim();
    
    if (comando.length() > 0) {
      procesarComandoLora(comando);
    }
  }
  
  delay(100);
}