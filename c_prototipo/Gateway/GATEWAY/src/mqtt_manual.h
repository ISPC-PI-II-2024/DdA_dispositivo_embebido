// mqtt_manual.h - Implementación manual de MQTT sobre WebSocket
#ifndef MQTT_MANUAL_H
#define MQTT_MANUAL_H

#include <Arduino.h>
#include <WebSocketsClient.h>

class MQTTManual {
private:
    WebSocketsClient* ws;
    String clientId;
    uint8_t buf[512];
    bool isConnected;
    
    // Codifica remaining length según protocolo MQTT
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
    
    // Envía MQTT CONNECT
    void connect() {
        int p = 0;
        buf[p++] = 0x10;  // CONNECT
        
        int clientLen = clientId.length();
        int remLen = 10 + 2 + clientLen;
        p += encodeLength(&buf[p], remLen);
        
        // Protocol: MQTT 3.1.1
        buf[p++] = 0x00; buf[p++] = 0x04;
        buf[p++] = 'M'; buf[p++] = 'Q'; buf[p++] = 'T'; buf[p++] = 'T';
        buf[p++] = 0x04;  // Version
        buf[p++] = 0x02;  // Clean session
        buf[p++] = 0x00; buf[p++] = 0x3C;  // Keepalive 60s
        
        // Client ID
        buf[p++] = (clientLen >> 8) & 0xFF;
        buf[p++] = clientLen & 0xFF;
        memcpy(&buf[p], clientId.c_str(), clientLen);
        p += clientLen;
        
        ws->sendBIN(buf, p);
        Serial.println("📤 MQTT CONNECT enviado");
    }
    
    // Publica un mensaje
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
    
    // Suscribe a un topic
    bool subscribe(const char* topic) {
        if (!isConnected) return false;
        
        int p = 0;
        buf[p++] = 0x82;  // SUBSCRIBE
        
        int topicLen = strlen(topic);
        int remLen = 2 + 2 + topicLen + 1;
        p += encodeLength(&buf[p], remLen);
        
        // Packet ID
        buf[p++] = 0x00; buf[p++] = 0x01;
        
        // Topic
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
    
    // Envía PING
    void ping() {
        uint8_t p[] = {0xC0, 0x00};
        ws->sendBIN(p, 2);
    }
    
    // Procesa mensajes MQTT recibidos
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
        else if (type == 3) {  // PUBLISH - Mensaje recibido
            int p = 1;
            while (data[p] & 0x80) p++;  // Skip remaining length
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
                }
            }
        }
        else if (type == 9) {  // SUBACK
            Serial.println("✅ SUBACK recibido");
        }
        else if (type == 13) {  // PINGRESP
            // Ping respondido, conexión activa
        }
    }
    
    bool connected() {
        return isConnected;
    }
    
    void setConnected(bool state) {
        isConnected = state;
    }
};

#endif