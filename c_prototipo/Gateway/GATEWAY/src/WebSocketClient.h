// WebSocketClient.h - Wrapper mejorado para MQTT sobre WebSocket
#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Client.h>
#include <WebSocketsClient.h>

class WebSocketClient : public Client {
private:
    WebSocketsClient* ws;
    static const size_t BUFFER_SIZE = 2048;
    uint8_t rxBuffer[BUFFER_SIZE];
    size_t rxBufferIndex;
    size_t rxBufferLen;
    bool wsConnected;

public:
    WebSocketClient(WebSocketsClient* websocket) 
        : ws(websocket), rxBufferIndex(0), rxBufferLen(0), wsConnected(false) {}

    int connect(IPAddress ip, uint16_t port) override {
        // Ya conectado por WebSocket
        return wsConnected ? 1 : 0;
    }

    int connect(const char *host, uint16_t port) override {
        // Ya conectado por WebSocket
        return wsConnected ? 1 : 0;
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t *buf, size_t size) override {
        if (ws && wsConnected) {
            ws->sendBIN(buf, size);
            return size;
        }
        return 0;
    }

    int available() override {
        return (rxBufferLen - rxBufferIndex);
    }

    int read() override {
        if (available() > 0) {
            return rxBuffer[rxBufferIndex++];
        }
        return -1;
    }

    int read(uint8_t *buf, size_t size) override {
        size_t count = 0;
        while (count < size && available() > 0) {
            buf[count++] = rxBuffer[rxBufferIndex++];
        }
        return count;
    }

    int peek() override {
        if (available() > 0) {
            return rxBuffer[rxBufferIndex];
        }
        return -1;
    }

    void flush() override {
        rxBufferIndex = 0;
        rxBufferLen = 0;
    }

    void stop() override {
        if (ws) {
            ws->disconnect();
        }
        wsConnected = false;
        flush();
    }

    uint8_t connected() override {
        if (ws) {
            wsConnected = ws->isConnected();
            return wsConnected ? 1 : 0;
        }
        return 0;
    }

    operator bool() override {
        return connected() != 0;
    }

    // Método para alimentar datos del WebSocket
    void feedData(uint8_t* payload, size_t length) {
        if (length > BUFFER_SIZE) {
            Serial.println("⚠️ Buffer overflow - datos truncados");
            length = BUFFER_SIZE;
        }
        
        // Si hay datos sin leer, los concatenamos
        if (rxBufferIndex < rxBufferLen) {
            size_t remaining = rxBufferLen - rxBufferIndex;
            memmove(rxBuffer, &rxBuffer[rxBufferIndex], remaining);
            rxBufferLen = remaining;
            rxBufferIndex = 0;
        } else {
            rxBufferLen = 0;
            rxBufferIndex = 0;
        }
        
        // Agregar nuevos datos
        if (rxBufferLen + length <= BUFFER_SIZE) {
            memcpy(&rxBuffer[rxBufferLen], payload, length);
            rxBufferLen += length;
        }
    }

    // Marcar WebSocket como conectado
    void setConnected(bool state) {
        wsConnected = state;
    }
};

#endif