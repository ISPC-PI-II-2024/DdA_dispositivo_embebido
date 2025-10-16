// WebSocketClient.h - Wrapper para usar WebSocket con PubSubClient
#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Client.h>
#include <WebSocketsClient.h>

class WebSocketClient : public Client {
private:
    WebSocketsClient* ws;
    uint8_t rxBuffer[1024];
    size_t rxBufferIndex;
    size_t rxBufferLen;

public:
    WebSocketClient(WebSocketsClient* websocket) : ws(websocket), rxBufferIndex(0), rxBufferLen(0) {}

    int connect(IPAddress ip, uint16_t port) override {
        return 1; // Ya conectado por WebSocket
    }

    int connect(const char *host, uint16_t port) override {
        return 1; // Ya conectado por WebSocket
    }

    size_t write(uint8_t b) override {
        return write(&b, 1);
    }

    size_t write(const uint8_t *buf, size_t size) override {
        if (ws) {
            ws->sendBIN(buf, size);
            return size;
        }
        return 0;
    }

    int available() override {
        if (rxBufferIndex < rxBufferLen) {
            return rxBufferLen - rxBufferIndex;
        }
        return 0;
    }

    int read() override {
        if (available()) {
            return rxBuffer[rxBufferIndex++];
        }
        return -1;
    }

    int read(uint8_t *buf, size_t size) override {
        size_t count = 0;
        while (count < size && available()) {
            buf[count++] = read();
        }
        return count;
    }

    int peek() override {
        if (available()) {
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
    }

    uint8_t connected() override {
        return ws ? ws->isConnected() : 0;
    }

    operator bool() override {
        return connected();
    }

    // Método para alimentar datos del WebSocket
    void feedData(uint8_t* payload, size_t length) {
        if (length > sizeof(rxBuffer)) {
            length = sizeof(rxBuffer);
        }
        memcpy(rxBuffer, payload, length);
        rxBufferLen = length;
        rxBufferIndex = 0;
    }
};

#endif