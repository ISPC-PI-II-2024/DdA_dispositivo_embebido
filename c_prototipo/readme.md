# Gateway MQTT con LoRa y Sensores Remotos

Sistema IoT multinivel con Gateway ESP32, endpoints LoRa y nodos sensores distribuidos por RS485. Recopila datos de temperatura y humedad de múltiples sensores remotos y los envía vía MQTT a un broker en la nube.

## 📋 Características

- ✅ Gateway ESP32 con conectividad WiFi y LoRa
- ✅ Comunicación LoRa 433MHz de largo alcance
- ✅ Bus RS485 para sensores distribuidos hasta 100m
- ✅ Protocolo MQTT 3.1.1 sobre WebSocket
- ✅ Portal web de configuración WiFi
- ✅ Display LCD 20x4 con información en tiempo real
- ✅ Sistema de polling inteligente de endpoints
- ✅ Publicación MQTT en 3 tópicos separados
- ✅ Monitoreo de batería y estado de carga
- ✅ Reset físico de configuración con botón
- ✅ Reconexión automática WiFi/MQTT/LoRa

## 🏗️ Arquitectura del Sistema

```
                       ☁️ MQTT Broker
                  (mqtt.ispciot.org:80)
                            ↑
                            | WebSocket
                            |
                   ┌────────┴────────┐
                   │  GATEWAY ESP32  │
                   │  - WiFi         │
                   │  - MQTT Client  │
                   │  - LCD 20x4     │
                   │  - LoRa RX/TX   │
                   └────────┬────────┘
                            | LoRa 433MHz
                            ↓
                   ┌────────────────┐
                   │ENDPOINT ESP32-C3│
                   │ - LoRa TX/RX    │
                   │ - RS485 Master  │
                   │ - 3 LEDs        │
                   │ - Batería       │
                   └────────┬────────┘
                            | RS485 (hasta 100m)
                            ↓
        ┌───────────────────┼───────────────────┐
        ↓                   ↓                   ↓
   ┌─────────┐        ┌─────────┐        ┌─────────┐
   │ESP8266-1│        │ESP8266-2│   ...  │ESP8266-N│
   │+ AHT10  │        │+ AHT10  │        │+ AHT10  │
   └─────────┘        └─────────┘        └─────────┘
  ID: 0F01           ID: 1Ab3           ID: 523A
```

### Flujo de Datos

1. **Endpoint** consulta sensores vía RS485
2. **Sensores ESP8266** responden con datos AHT10
3. **Endpoint** envía datos consolidados por LoRa
4. **Gateway** recibe, procesa y almacena en memoria
5. **Gateway** publica a MQTT en 3 tópicos diferentes

## 🔧 Hardware Requerido

### Gateway (ESP32 DevKit V1)
- **ESP32 DevKit V1** (Dual Core, WiFi integrado)
- **Módulo LoRa RA-02** (SX1278, 433MHz)
- **LCD I2C 20x4** (PCF8574 o equivalente)
- **Botón pulsador** (Normally Open)
- Cable USB y fuente 5V

### Endpoint (ESP32-C3)
- **ESP32-C3 DevKit** (RISC-V, bajo consumo)
- **Módulo LoRa SX1278** (433MHz)
- **Módulo MAX485** (RS485 transceiver)
- **3 LEDs** (Rojo, Amarillo, Verde) + resistencias 220Ω
- **Batería LiPo** 3.7V + módulo de carga TP4056
- **Divisor de voltaje** para medición de batería

### Nodo Sensor (ESP8266 × N)
- **ESP8266 NodeMCU v2** o Wemos D1 Mini
- **Sensor AHT10** (I2C, temperatura y humedad)
- **Módulo MAX485** (RS485 transceiver)
- Cable par trenzado para RS485
- Fuente 5V o alimentación por bus

## 📦 Dependencias

### Gateway
```ini
[env:gateway]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
lib_deps =
  bblanchon/ArduinoJson@^7.4.2
  marcoschwartz/LiquidCrystal_I2C@^1.1.4
  links2004/WebSockets@^2.7.1
  sandeepmistry/LoRa@^0.8.0
monitor_speed = 115200
```

### Endpoint
```ini
[env:endpoint]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps =
  sandeepmistry/LoRa@^0.8.0
monitor_speed = 115200
```

### Nodo Sensor
```ini
[env:sensor_node]
platform = espressif8266
board = nodemcuv2
framework = arduino
lib_deps =
  adafruit/Adafruit AHTX0@^2.0.5
  adafruit/Adafruit BusIO@^1.16.1
monitor_speed = 9600
```

## 🔌 Conexiones Hardware

### Gateway - LoRa RA-02
| LoRa RA-02 | ESP32 |
|------------|-------|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| NSS (CS) | GPIO 5 |
| RST | GPIO 26 |
| DIO0 | GPIO 27 |

### Gateway - LCD I2C
| LCD I2C | ESP32 |
|---------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Gateway - Botón Reset
| Componente | Pin |
|------------|-----|
| Terminal 1 | GPIO 33 |
| Terminal 2 | 3.3V |

### Endpoint - LoRa
| LoRa SX1278 | ESP32-C3 |
|-------------|----------|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO 4 |
| MISO | GPIO 5 |
| MOSI | GPIO 6 |
| CS | GPIO 7 |
| DIO0 | GPIO 3 |

### Endpoint - MAX485
| MAX485 | ESP32-C3 | Notas |
|--------|----------|-------|
| VCC | 5V | |
| GND | GND | |
| RO | GPIO 20 | Receiver Out (RX) |
| DI | GPIO 21 | Driver In (TX) |
| DE | GPIO 10 | Driver Enable |
| RE | GPIO 10 | Receiver Enable (unido a DE) |
| A | Bus A | Línea diferencial + |
| B | Bus B | Línea diferencial - |

### Endpoint - LEDs
| LED | GPIO ESP32-C3 | Función |
|-----|---------------|---------|
| Verde | GPIO 10 | LoRa conectado |
| Amarillo | GPIO 9 | Procesando comando |
| Rojo | GPIO 8 | Transmitiendo respuesta |

### Nodo Sensor - MAX485
| MAX485 | ESP8266 | Pin |
|--------|---------|-----|
| VCC | 5V | Vin |
| GND | GND | GND |
| RO | RX | D6 |
| DI | TX | D7 |
| DE/RE | Control | D5 |
| A | Bus A | Cable verde |
| B | Bus B | Cable blanco |

### Nodo Sensor - AHT10
| AHT10 | ESP8266 | Pin |
|-------|---------|-----|
| VCC | 3.3V | 3V3 |
| GND | GND | GND |
| SDA | I2C Data | D2 (GPIO 4) |
| SCL | I2C Clock | D1 (GPIO 5) |

### Bus RS485
```
Endpoint ──┬── ESP8266-1 ──┬── ESP8266-2 ── ... ── ESP8266-N
 MAX485    │    MAX485     │    MAX485              MAX485
   A ──────┼──────A────────┼──────A──────────────────A
   B ──────┼──────B────────┼──────B──────────────────B
           │               │
         120Ω            120Ω (opcional)
    (terminación)   (puntos intermedios)
```

**Notas importantes:**
- Cable **par trenzado** categoría 5 o superior
- Resistencias de **terminación 120Ω** en extremos
- Longitud máxima: **100 metros** por segmento
- Topología: **bus lineal** (no estrella)

## 🚀 Instalación y Configuración

### 1. Clonar Repositorio

```bash
git clone https://github.com/ISPC-PI-II-2024/DdA_dispositivo_embebido.git
cd DdA_dispositivo_embebido
```

### 2. Compilar y Subir Gateway

```bash
cd gateway_mqtt
pio run --target upload --environment gateway
pio device monitor --environment gateway
```

### 3. Compilar y Subir Endpoint

```bash
cd endpoint_lora
# ⚠️ EDITAR src/main.cpp y cambiar ENDPOINT_ID a "E01", "E02", etc.
pio run --target upload --environment endpoint
pio device monitor --environment endpoint
```

### 4. Compilar y Subir Nodos Sensores

```bash
cd sensor_node
# ⚠️ EDITAR src/main.cpp y cambiar SENSOR_ID (1, 2, 3, ...)
pio run --target upload --environment sensor_node
pio device monitor --environment sensor_node
```

## 📱 Configuración WiFi (Primera vez)

### Paso 1: Modo Access Point
El gateway crea un AP:
- **SSID**: `GatewayMQTT_Gat_01`
- **IP**: `192.168.4.1`
- **Duración**: 3 minutos

### Paso 2: Portal Web
1. Conectarse al AP desde móvil/PC
2. Abrir navegador en `http://192.168.4.1`
3. Ingresar credenciales WiFi
4. Guardar → Reinicio automático

### Paso 3: Operación Normal
- Gateway se conecta automáticamente
- Inicia MQTT y LoRa
- Comienza polling de endpoints

## 🔄 Reset de Configuración

**Mantener botón GPIO 33 por 3 segundos:**

```
LCD muestra:
Reset WiFi: 1/3
████────────────────

Reset WiFi: 2/3
████████────────────

Reset WiFi: 3/3
████████████████████

→ Borrando WiFi
  Reiniciando...
```

**Soltar antes de 3s:** Cancela operación

## 📡 Protocolos de Comunicación

### RS485 (Endpoint ↔ Nodos Sensores)

**Baud Rate:** 9600  
**Formato:** 8N1  
**Timeout:** 1000ms

#### Comando (Master → Slave)
```
READ:1\n
```
Donde `1` es el ID del sensor (1-10)

#### Respuesta OK (Slave → Master)
```
SENSOR:1|ID:0F01|POS:1|TEMP:25.5|HUM:62.0|STATE:ok\n
```

#### Respuesta Error
```
ERROR:1\n
```

### LoRa (Endpoint ↔ Gateway)

**Frecuencia:** 433 MHz  
**SF:** 7 (Spreading Factor)  
**BW:** 125 kHz  
**CR:** 4/5 (Coding Rate)  
**Sync Word:** 0x12  
**Potencia:** 17 dBm

#### Comando 1: Listar Endpoints
```
Gateway TX: LIST_ENDPOINTS
Endpoint RX: (todos los endpoints escuchan)
Endpoint TX: ENDPOINTS:E01
```

#### Comando 2: Solicitar Datos
```
Gateway TX: GET_DATA:E01
Endpoint RX: (solo responde E01)
Endpoint TX: DATA:E01|BAT:99|CHG:1|SNS:3|S1:0F01,1,17.7,62,ok|S2:1Ab3,2,20.8,63,ok|S3:523A,3,19.2,58,ok
```

**Formato de sensor:**
```
SN:ID,POSICION,TEMPERATURA,HUMEDAD,ESTADO
```

### MQTT (Gateway → Broker)

**Broker:** mqtt.ispciot.org:80  
**Protocolo:** MQTT 3.1.1 sobre WebSocket  
**Path:** `/mqtt`  
**QoS:** 0 (sin confirmación)

#### Tópico 1: `gateway/gateway`
Estado del gateway (cada 30s)

```json
{
  "id_gateway": "G01",
  "wifi_signal": "excelente",
  "lora_status": "ok",
  "uptime": "19:47:23"
}
```

#### Tópico 2: `gateway/endpoint`
Estado de endpoints (cada 60s)

```json
{
  "id_gateway": "G01",
  "endpoints": [
    {
      "id": "E01",
      "bateria": 99,
      "cargando": true,
      "lora": "ok",
      "sensores": 3
    }
  ]
}
```

#### Tópico 3: `gateway/sensor`
Datos de sensores (cada 60s)

```json
{
  "id_gateway": "G01",
  "endpoints": [
    {
      "id_endpoint": "E01",
      "sensores": [
        {
          "id": "0F01",
          "posicion": 1,
          "temp": 17.7,
          "humedad": 62,
          "estado": "ok"
        },
        {
          "id": "1Ab3",
          "posicion": 2,
          "temp": 20.8,
          "humedad": 63,
          "estado": "ok"
        }
      ]
    }
  ]
}
```

## 📺 Display LCD 20x4

### Operación Normal
```
┌────────────────────┐
│WiFi: Conectado     │
│MQTT: Conectado     │
│LoRa: ok            │
│Endpoints: 3        │
└────────────────────┘
```

### Modo AP
```
┌────────────────────┐
│Modo AP Activo      │
│SSID: GatewayMQTT   │
│IP: 192.168.4.1     │
│Config: Web Portal  │
└────────────────────┘
```

### Iniciando
```
┌────────────────────┐
│Gateway MQTT        │
│Iniciando...        │
│                    │
│                    │
└────────────────────┘
```

## ⏱️ Timings del Sistema

| Componente | Acción | Intervalo |
|------------|--------|-----------|
| **Gateway** | Polling LoRa | 45s |
| **Gateway** | Publicar gateway/gateway | 30s |
| **Gateway** | Publicar gateway/endpoint | 60s |
| **Gateway** | Publicar gateway/sensor | 60s |
| **Gateway** | MQTT ping | 30s |
| **Gateway** | LCD update | 2s |
| **Endpoint** | Check LoRa | 10s |
| **Endpoint** | Timeout comando | 5s |
| **Nodo Sensor** | Respuesta RS485 | <1s |

## 📊 Uso de Recursos

### Gateway ESP32
- **RAM:** ~18% (60KB / 327KB)
- **Flash:** ~78% (1,020KB / 1,310KB)
- **Componentes principales:**
  - LoRa manager: ~8KB
  - Cliente MQTT: ~5KB
  - LCD con caché: ~2KB
  - Portal web: ~3KB

### Endpoint ESP32-C3
- **RAM:** ~12% (40KB / 327KB)
- **Flash:** ~65% (850KB / 1,310KB)

### Nodo Sensor ESP8266
- **RAM:** ~25% (20KB / 80KB)
- **Flash:** ~45% (450KB / 1MB)

## 🧪 Pruebas Unitarias Implementadas

Este proyecto sirve como prueba de concepto IoT multinivel con:

### Conectividad
- [x] Portal cautivo de configuración WiFi
- [x] Persistencia de credenciales (NVS/Preferences)
- [x]  Reconexión automática WiFi
- [x] Cliente MQTT manual sobre WebSocket
- [x]  Publicación periódica de mensajes MQTT (3 tópicos)
- [x] Suscripción y recepción de mensajes MQTT
- [x] Keepalive MQTT (PING/PONG)
- [x] Timeout de Access Point (3 minutos)

### LoRa
- [x] Comunicación bidireccional LoRa 433MHz
- [x] Sistema de polling de endpoints
- [x] Comando/respuesta con timeout
- [x] Verificación de integridad CRC
- [x] Medición de RSSI y SNR
- [x] Reconexión automática LoRa

### RS485
- [ ] Comunicación half-duplex RS485
- [ ] Protocolo master/slave
- [ ] Control DE/RE del transceiver MAX485
- [x] Detección de timeout
- [x] Múltiples slaves en bus

### Sensores
- [x] Lectura I2C de sensores AHT10
- [x] Validación de rangos de temperatura/humedad
- [ ] Manejo de errores de comunicación
- [x] Identificación única de sensores

### Interfaz
- [x] Display LCD 20x4 con sistema anti-parpadeo
- [x] Manejo de estados temporales en LCD
- [x] Reset físico con botón y feedback visual
- [x] Barra de progreso en LCD
- [x] Indicadores LED de estado

### Gestión de Energía
- [ ] Medición de nivel de batería (ADC)
- [ ] Detección de estado de carga
- [ ] Publicación de estado energético

### Tiempo
- [ ] Sincronización NTP con zona horaria Argentina
- [ ] Cálculo de uptime del sistema
- [x] Timestamps en mensajes MQTT

## 📁 Estructura del Proyecto

```
DdA_dispositivo_embebido/
├── gateway_mqtt/
│   ├── src/
│   │   ├── main.cpp              # Loop principal gateway
│   │   ├── conexiones.cpp/.h     # WiFi, MQTT, WebSocket
│   │   ├── lcdplus.cpp/.h        # Control LCD 20x4
│   │   ├── lora_manager.cpp/.h   # Gestión LoRa y endpoints
│   │   ├── mqtt_manual.h         # Cliente MQTT manual
│   │   └── web_portal.cpp/.h     # Servidor web AP
│   └── platformio.ini
│
├── endpoint_lora/
│   ├── src/
│   │   └── main.cpp              # Endpoint con LoRa y RS485
│   └── platformio.ini
│
├── sensor_node/
│   ├── src/
│   │   └── main.cpp              # Nodo sensor AHT10
│   └── platformio.ini
│
├── docs/
│   ├── TROUBLESHOOTING.md        # Solución de problemas
│   ├── TECHNICAL_NOTES.md        # Notas técnicas
│   └── diagrams/                 # Diagramas y esquemas
│
└── README.md                     # Este archivo
```

## ⚙️ Personalización

### Cambiar ID del Gateway
```cpp
// gateway_mqtt/src/conexiones.cpp
const char* gatewayId = "G01";  // Cambiar aquí
```

### Cambiar ID del Endpoint
```cpp
// endpoint_lora/src/main.cpp
#define ENDPOINT_ID "E01"  // E01, E02, E03, etc.
```

### Cambiar ID del Sensor
```cpp
// sensor_node/src/main.cpp
#define SENSOR_ID 1  // 1, 2, 3, 4, 5, etc.
```

### Ajustar Intervalos del Gateway
```cpp
// gateway_mqtt/src/main.cpp
const unsigned long publishIntervalGateway = 30000;   // 30s
const unsigned long publishIntervalEndpoints = 60000; // 60s
const unsigned long publishIntervalSensors = 60000;   // 60s
const unsigned long loraUpdateInterval = 45000;       // 45s polling
```

### Cambiar Frecuencia LoRa
```cpp
// En todos los dispositivos LoRa
#define LORA_FREQ 433E6  // 433 MHz (cambiar a 915E6 para 915MHz)
```

### Configurar Dirección LCD
```cpp
// gateway_mqtt/src/lcdplus.cpp
static LiquidCrystal_I2C lcd(0x27, 20, 4);  // 0x27 o 0x3F
```

## 📄 Licencia

Este proyecto es parte del trabajo práctico de **Proyecto Integrador II** del ISPC.

**Institución:** Instituto Superior Politécnico Córdoba  
**Carrera:** Tecnicatura Superior en Telecomunicaciones  
**Año:** 2024

## 👥 Contribuciones

Proyecto desarrollado por estudiantes del ISPC como prueba de concepto de sistema IoT multinivel.

## 🙏 Agradecimientos

- Biblioteca **LoRa** de [Sandeep Mistry](https://github.com/sandeepmistry/arduino-LoRa)
- Biblioteca **WebSockets** de [Links2004](https://github.com/Links2004/arduinoWebSockets)
- Biblioteca **LiquidCrystal_I2C** de [marcoschwartz](https://github.com/marcoschwartz/LiquidCrystal_I2C)
- Biblioteca **ArduinoJson** de [bblanchon](https://github.com/bblanchon/ArduinoJson)
- Biblioteca **Adafruit AHTX0** de [Adafruit](https://github.com/adafruit/Adafruit_AHTX0)
- Comunidad de PlatformIO y ESP32/ESP8266

## 📧 Contacto

**Repositorio:** https://github.com/ISPC-PI-II-2024/DdA_dispositivo_embebido

---

**Proyecto:** Sistema IoT Multinivel con Gateway MQTT  
**Versión:** 2.0.0  
**Última actualización:** Octubre 2025  
**Estado:** En desarrollo activo
