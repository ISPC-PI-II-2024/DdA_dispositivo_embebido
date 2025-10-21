#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ==================== CONFIGURACIÓN ====================
const char* GATEWAY_ID = "ESP32-Gateway";
const unsigned long RECONNECT_INTERVAL = 10000;
const unsigned long STATUS_INTERVAL = 30000;
const unsigned long HEARTBEAT_INTERVAL = 25000;
const unsigned long AP_TIMEOUT = 180000;

// ==================== MQTT MANUAL (Integrado) ====================
class MQTTManual {
private:
    WebSocketsClient* ws;
    String clientId;
    uint8_t buf[512];
    bool isConnected;
    
    int encodeLength(uint8_t* b, int len) {
        int i = 0;
        do {
            uint8_t d = len % 128;
            len /= 128;
            if (len > 0) d |= 0x80;
            b[i++] = d;
        } while (len > 0);
        return i;
    }

public:
    MQTTManual(WebSocketsClient* websocket, const char* id) 
        : ws(websocket), clientId(id), isConnected(false) {}
    
    void connect() {
        int p = 0;
        buf[p++] = 0x10;  // CONNECT
        
        int clientLen = clientId.length();
        int remLen = 10 + 2 + clientLen;
        p += encodeLength(&buf[p], remLen);
        
        buf[p++] = 0x00; buf[p++] = 0x04;
        buf[p++] = 'M'; buf[p++] = 'Q'; buf[p++] = 'T'; buf[p++] = 'T';
        buf[p++] = 0x04;  // Version
        buf[p++] = 0x02;  // Clean session
        buf[p++] = 0x00; buf[p++] = 0x3C;  // Keepalive 60s
        
        buf[p++] = (clientLen >> 8) & 0xFF;
        buf[p++] = clientLen & 0xFF;
        memcpy(&buf[p], clientId.c_str(), clientLen);
        p += clientLen;
        
        ws->sendBIN(buf, p);
        Serial.println("📤 MQTT CONNECT enviado");
    }
    
    bool publish(const char* topic, const char* msg) {
        if (!isConnected) return false;
        
        int p = 0;
        buf[p++] = 0x30;  // PUBLISH QoS 0
        
        int topicLen = strlen(topic);
        int msgLen = strlen(msg);
        int remLen = 2 + topicLen + msgLen;
        p += encodeLength(&buf[p], remLen);
        
        buf[p++] = (topicLen >> 8) & 0xFF;
        buf[p++] = topicLen & 0xFF;
        memcpy(&buf[p], topic, topicLen);
        p += topicLen;
        
        memcpy(&buf[p], msg, msgLen);
        p += msgLen;
        
        ws->sendBIN(buf, p);
        Serial.print("📤 Publicado [");
        Serial.print(topic);
        Serial.print("]: ");
        Serial.println(msg);
        return true;
    }
    
    bool subscribe(const char* topic) {
        if (!isConnected) return false;
        
        int p = 0;
        buf[p++] = 0x82;  // SUBSCRIBE
        
        int topicLen = strlen(topic);
        int remLen = 2 + 2 + topicLen + 1;
        p += encodeLength(&buf[p], remLen);
        
        buf[p++] = 0x00; buf[p++] = 0x01;
        
        buf[p++] = (topicLen >> 8) & 0xFF;
        buf[p++] = topicLen & 0xFF;
        memcpy(&buf[p], topic, topicLen);
        p += topicLen;
        
        buf[p++] = 0x00;  // QoS 0
        
        ws->sendBIN(buf, p);
        Serial.print("📤 Suscrito a: ");
        Serial.println(topic);
        return true;
    }
    
    void ping() {
        uint8_t p[] = {0xC0, 0x00};
        ws->sendBIN(p, 2);
    }
    
    void processMessage(uint8_t* data, size_t len) {
        if (len < 2) return;
        
        uint8_t type = (data[0] >> 4) & 0x0F;
        
        if (type == 2) {  // CONNACK
            if (data[3] == 0) {
                Serial.println("✅ MQTT Conectado!");
                isConnected = true;
            } else {
                Serial.printf("❌ CONNACK error: %d\n", data[3]);
            }
        }
        else if (type == 3) {  // PUBLISH
            int p = 1;
            while (data[p] & 0x80) p++;
            p++;
            
            int topicLen = (data[p] << 8) | data[p+1];
            p += 2;
            
            char topic[64];
            if (topicLen < 64) {
                memcpy(topic, &data[p], topicLen);
                topic[topicLen] = '\0';
                p += topicLen;
                
                int msgLen = len - p;
                if (msgLen > 0 && msgLen < 256) {
                    char msg[256];
                    memcpy(msg, &data[p], msgLen);
                    msg[msgLen] = '\0';
                    
                    Serial.print("📥 Recibido [");
                    Serial.print(topic);
                    Serial.print("]: ");
                    Serial.println(msg);
                    
                    // Manejar comandos recibidos
                    if (String(topic) == "gateway/control") {
                        if (String(msg) == "reset") {
                            Serial.println("🔄 Comando reset recibido");
                            ESP.restart();
                        }
                    }
                }
            }
        }
        else if (type == 9) {  // SUBACK
            Serial.println("✅ SUBACK recibido");
        }
        else if (type == 13) {  // PINGRESP
            Serial.println("💓 PINGRESP recibido");
        }
    }
    
    bool connected() { return isConnected; }
    void setConnected(bool state) { isConnected = state; }
};

// ==================== VARIABLES GLOBALES ====================
Preferences preferences;
WebSocketsClient webSocket;
MQTTManual* mqttClient = nullptr;

// Variables de estado
String wifiSSID = "";
String wifiPassword = "";
String mqttServer = "broker.emqx.io";
int mqttPort = 8083;
bool wifiConfigured = false;
bool apMode = false;
unsigned long apStartTime = 0;
unsigned long lastReconnect = 0;
unsigned long lastStatus = 0;
unsigned long lastHeartbeat = 0;
int reconnectCount = 0;

enum Estado { 
    ESTADO_AP, 
    ESTADO_CONECTANDO_WIFI, 
    ESTADO_CONECTANDO_WS, 
    ESTADO_CONECTANDO_MQTT, 
    ESTADO_OPERATIVO,
    ESTADO_RECONECTANDO
};
Estado estadoActual = ESTADO_AP;

// ==================== LCD SIMPLIFICADO ====================
void mostrarLCD(String linea1, String linea2) {
    Serial.println("📟 LCD: " + linea1 + " | " + linea2);
    // Aquí integrarías tu código LCD real
}

void actualizarLCD() {
    switch(estadoActual) {
        case ESTADO_AP:
            mostrarLCD("MODO CONFIGURACION", "Conectarse al AP");
            break;
        case ESTADO_CONECTANDO_WIFI:
            mostrarLCD("CONECTANDO WIFI", wifiSSID);
            break;
        case ESTADO_CONECTANDO_WS:
            mostrarLCD("CONECTANDO WS", mqttServer);
            break;
        case ESTADO_CONECTANDO_MQTT:
            mostrarLCD("CONECTANDO MQTT", "Autenticando...");
            break;
        case ESTADO_OPERATIVO:
            mostrarLCD("SISTEMA OPERATIVO", WiFi.localIP().toString());
            break;
        case ESTADO_RECONECTANDO:
            mostrarLCD("RECONECTANDO...", "Intento: " + String(reconnectCount));
            break;
    }
}

// ==================== FUNCIONES WiFi ====================
void cargarConfigWiFi() {
    preferences.begin("gateway", true);
    wifiSSID = preferences.getString("ssid", "");
    wifiPassword = preferences.getString("password", "");
    mqttServer = preferences.getString("mqtt_server", "broker.emqx.io");
    mqttPort = preferences.getInt("mqtt_port", 8083);
    preferences.end();
    
    wifiConfigured = (wifiSSID.length() > 0);
    Serial.println("📋 Config WiFi: " + String(wifiConfigured ? "Cargada" : "No configurada"));
}

void guardarConfigWiFi(String ssid, String pass) {
    preferences.begin("gateway", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);
    preferences.end();
    
    wifiSSID = ssid;
    wifiPassword = pass;
    wifiConfigured = true;
    Serial.println("💾 WiFi guardado: " + ssid);
}

void iniciarAP() {
    Serial.println("📡 Iniciando Modo AP...");
    WiFi.softAP("ESP32-Gateway", NULL);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    mostrarLCD("AP: ESP32-Gateway", WiFi.softAPIP().toString());
    apMode = true;
    apStartTime = millis();
    estadoActual = ESTADO_AP;
}

void conectarWiFi() {
    Serial.println("📶 Conectando a: " + wifiSSID);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
    estadoActual = ESTADO_CONECTANDO_WIFI;
    lastReconnect = millis();
}

// ==================== WebSocket & MQTT ====================
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_CONNECTED:
            Serial.println("✅ WebSocket conectado");
            mostrarLCD("WS: CONECTADO", "Iniciando MQTT...");
            
            // Inicializar MQTT después de WS conectado
            if (!mqttClient) {
                mqttClient = new MQTTManual(&webSocket, GATEWAY_ID);
            }
            mqttClient->connect();
            estadoActual = ESTADO_CONECTANDO_MQTT;
            break;
            
        case WStype_DISCONNECTED:
            Serial.println("❌ WebSocket desconectado");
            if (mqttClient) mqttClient->setConnected(false);
            estadoActual = ESTADO_RECONECTANDO;
            break;
            
        case WStype_BIN:
            if (mqttClient) {
                mqttClient->processMessage(payload, length);
                
                // Si MQTT se conecta exitosamente
                if (estadoActual == ESTADO_CONECTANDO_MQTT && mqttClient->connected()) {
                    Serial.println("🎉 Sistema completamente operativo!");
                    mostrarLCD("SISTEMA LISTO", "Gateway operativo");
                    estadoActual = ESTADO_OPERATIVO;
                    
                    // Suscribirse a topics
                    mqttClient->subscribe("gateway/control");
                    mqttClient->subscribe("sensors/+/data");
                }
            }
            break;
            
        case WStype_ERROR:
            Serial.println("❌ Error WebSocket");
            break;
    }
}

void conectarWebSocket() {
    Serial.println("🔗 Conectando WebSocket a: " + mqttServer + ":" + String(mqttPort));
    webSocket.begin(mqttServer.c_str(), mqttPort, "/mqtt");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    estadoActual = ESTADO_CONECTANDO_WS;
}

// ==================== PUBLICACIÓN DE DATOS ====================
void publicarEstado() {
    if (estadoActual != ESTADO_OPERATIVO || !mqttClient || !mqttClient->connected()) return;
    
    String estado = "{";
    estado += "\"id\":\"" + String(GATEWAY_ID) + "\",";
    estado += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    estado += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    estado += "\"reconexiones\":" + String(reconnectCount) + ",";
    estado += "\"heapLibre\":" + String(ESP.getFreeHeap()) + ",";
    estado += "\"estado\":\"OPERATIVO\"";
    estado += "}";
    
    mqttClient->publish("gateway/estado", estado.c_str());
}

void publicarDatosSensor(const char* tipoSensor, float temperatura, float humedad) {
    if (estadoActual != ESTADO_OPERATIVO || !mqttClient || !mqttClient->connected()) return;
    
    String datos = "{";
    datos += "\"sensor\":\"" + String(tipoSensor) + "\",";
    datos += "\"temperatura\":" + String(temperatura) + ",";
    datos += "\"humedad\":" + String(humedad) + ",";
    datos += "\"timestamp\":" + String(millis());
    datos += "}";
    
    String topic = "sensores/" + String(tipoSensor) + "/datos";
    mqttClient->publish(topic.c_str(), datos.c_str());
}

// ==================== RECONEXIÓN INTELIGENTE ====================
void manejarReconexion() {
    unsigned long ahora = millis();
    
    if (ahora - lastReconnect >= RECONNECT_INTERVAL) {
        Serial.println("🔄 Intentando reconexión...");
        reconnectCount++;
        
        WiFi.disconnect();
        webSocket.disconnect();
        if (mqttClient) {
            mqttClient->setConnected(false);
        }
        
        estadoActual = ESTADO_CONECTANDO_WIFI;
        conectarWiFi();
        lastReconnect = ahora;
    }
}

// ==================== SETUP PRINCIPAL ====================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n🚀 Gateway MQTT/WebSocket Iniciado");
    Serial.println("ID: " + String(GATEWAY_ID));
    
    // Cargar configuración
    cargarConfigWiFi();
    
    if (wifiConfigured) {
        // Intentar conexión WiFi
        conectarWiFi();
    } else {
        // Modo AP para configuración
        iniciarAP();
    }
    
    mostrarLCD("INICIANDO", "Sistema gateway...");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
    unsigned long ahora = millis();
    
    // Manejar diferentes estados
    switch(estadoActual) {
        case ESTADO_AP:
            // Timeout del AP después de 3 minutos
            if (ahora - apStartTime >= AP_TIMEOUT) {
                Serial.println("⏰ Timeout AP - Reiniciando...");
                ESP.restart();
            }
            break;
            
        case ESTADO_CONECTANDO_WIFI:
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("✅ WiFi conectado! IP: " + WiFi.localIP().toString());
                conectarWebSocket();
            } else if (ahora - lastReconnect > RECONNECT_INTERVAL) {
                Serial.println("❌ WiFi falló - Reiniciando...");
                ESP.restart();
            }
            break;
            
        case ESTADO_CONECTANDO_WS:
            webSocket.loop();
            break;
            
        case ESTADO_CONECTANDO_MQTT:
            webSocket.loop();
            break;
            
        case ESTADO_OPERATIVO:
            webSocket.loop();
            
            // Publicar heartbeat cada 25s
            if (ahora - lastHeartbeat >= HEARTBEAT_INTERVAL) {
                if (mqttClient && mqttClient->connected()) {
                    mqttClient->ping();
                    Serial.println("💓 PING enviado");
                }
                lastHeartbeat = ahora;
            }
            
            // Publicar estado cada 30s
            if (ahora - lastStatus >= STATUS_INTERVAL) {
                publicarEstado();
                
                // Simular datos de sensor (reemplazar con sensores reales)
                publicarDatosSensor("DHT22", random(200, 300)/10.0, random(400, 800)/10.0);
                
                lastStatus = ahora;
            }
            break;
            
        case ESTADO_RECONECTANDO:
            manejarReconexion();
            break;
    }
    
    // Actualizar LCD cada 2 segundos
    static unsigned long ultimoLCD = 0;
    if (ahora - ultimoLCD >= 2000) {
        actualizarLCD();
        ultimoLCD = ahora;
    }
    
    delay(10);
}