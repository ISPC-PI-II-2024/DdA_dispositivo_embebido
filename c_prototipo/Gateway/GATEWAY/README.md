# 🚀 Gateway MQTT + LoRa v1.2

Sistema de **gateway MQTT sobre WebSocket** con soporte **LoRa**, desarrollado para **ESP32**, con configuración WiFi mediante portal web, visualización en pantalla LCD y botón físico de reset.  
Permite recibir datos desde nodos LoRa y retransmitirlos hacia un servidor MQTT, manteniendo conectividad constante y estado visible en pantalla.

---

## 🧩 Arquitectura General

El sistema combina comunicación **LoRa**, **WiFi**, **MQTT sobre WebSocket** y una **interfaz física de usuario** (LCD + botón).  
Cada módulo tiene una función específica pero trabajan de forma sincronizada:

```
┌──────────────────────────────────────────────────┐
│                    ESP32                         │
│                                                  │
│  ┌────────────┐   ┌──────────────┐   ┌────────┐  │
│  │  LoRa RX/TX│ → │ MQTT/WebSock │ → │ Broker │  │
│  └────────────┘   └──────────────┘   └────────┘  │
│         │                 ↑                      │
│         │                 │                      │
│  ┌────────────┐     ┌──────────────┐             │
│  │ LCD I2C    │ ←→  │ WiFi Manager │             │
│  └────────────┘     └──────────────┘             │
│         │                                        │
│  ┌────────────┐                                  │
│  │ Botón Reset│                                  │
│  └────────────┘                                  │
└──────────────────────────────────────────────────┘
```

- Los **nodos LoRa** transmiten mensajes al gateway.  
- El gateway **procesa y publica los datos** vía MQTT.  
- El sistema **mantiene reconexión automática** de WiFi y MQTT.  
- El **LCD** refleja el estado del sistema sin parpadeos.  
- Un **botón físico** permite borrar configuración WiFi y reconfigurar.

---

## ⚙️ Modularidad del Código

| Archivo | Descripción |
|----------|--------------|
| **`main.cpp`** | Punto de entrada principal. Inicializa periféricos, WiFi, MQTT, LCD y LoRa. Controla el ciclo de publicación y reconexión. |
| **`conexiones.h / conexiones.cpp`** | Módulo de red: WiFi, MQTT, WebSocket, NTP y gestión de reconexiones. También maneja el botón de reset WiFi. |
| **`lcdplus.h / lcdplus.cpp`** | Control del LCD I2C con sistema anti-parpadeo mediante caché de líneas y mensajes temporales. |
| **`lora_manager.h / lora_manager.cpp`** | Comunicación LoRa (recepción y envío). Decodifica los paquetes entrantes y los reenvía al sistema principal. |
| **`mqtt_manual.h`** | Implementación manual del protocolo MQTT 3.1.1 sobre WebSocket. No depende de PubSubClient. |
| **`web_portal.cpp`** | Implementa el servidor web para la configuración WiFi inicial (modo AP). |
| **`WebSocketClient.h`** | Adaptador de bajo nivel entre el socket TCP y el protocolo WebSocket. |

---

## 🧠 Diagrama de Flujo del Sistema

```
           ┌────────────────────┐
           │      Inicio         │
           └───────┬────────────┘
                   │
                   ▼
          ¿WiFi configurado?
             ┌──────┴───────┐
             │ Sí            │ No
             ▼               ▼
     Conectar WiFi     Activar Modo AP
          │             Esperar configuración
          ▼
    ¿Conexión exitosa?
       ┌──┴───┐
       │ Sí   │ No
       ▼      ▼
 Conectar MQTT  Reintentar WiFi
       │
       ▼
Iniciar LoRa + LCD
       │
       ▼
┌──────────────────────────┐
│ Ciclo principal:         │
│ - Recibir LoRa           │
│ - Publicar MQTT          │
│ - Actualizar LCD         │
│ - Ping MQTT / NTP        │
│ - Monitorear botón reset │
└──────────────────────────┘
       │
       ▼
     Fin / Reset
```

---

## 📖 Manual de Usuario

### 🔧 Primer uso

1. **Encender el dispositivo**: si no hay WiFi configurado, el ESP32 crea un punto de acceso:
   ```
   SSID: GatewayMQTT_Gat_01
   IP: 192.168.4.1
   ```
2. **Conectarse desde el celular o PC** al AP `GatewayMQTT_Gat_01`.
3. Abrir un navegador y acceder a:  
   👉 `http://192.168.4.1`
4. Ingresar el **SSID y contraseña** de la red WiFi.
5. Presionar “Guardar y conectar”.  
   El dispositivo se reiniciará y conectará automáticamente.

---

### 🌐 Operación normal

Una vez conectado:
- El **LCD** mostrará:
  ```
  WiFi: Conectado
  MQTT: Conectado
  LoRa: Escuchando
  ```
- El gateway comenzará a:
  - Recibir tramas LoRa.  
  - Publicar su estado cada 30 segundos.  
  - Mantener el enlace MQTT con PING cada 30 segundos.

---

### 🔄 Reset WiFi

Para borrar la configuración guardada:
1. Mantener presionado el **botón físico (GPIO 33)** durante **3 segundos**.
2. La pantalla mostrará una barra de progreso:
   ```
   Reset WiFi: 1/3
   ████────────────
   ```
3. Al completar, se borra la configuración y el equipo entra en **modo AP**.

Si se suelta antes:
```
Cancelado
```

---

### 📡 Comunicación LoRa ↔ MQTT

Cuando un **nodo LoRa** envía datos al gateway, este los reempaqueta y los publica en el broker MQTT.

#### 🔹 Ejemplo de trama LoRa recibida:
```
<LoRa> TEMP=24.5,HUM=60,ID=NODE_A
```

#### 🔹 Conversión a JSON MQTT:
Publicado en:
```
Topic: gateway/Gat_01/lora/NODE_A
```
Payload:
```json
{
  "gateway_id": "Gat_01",
  "node_id": "NODE_A",
  "temp": 24.5,
  "hum": 60,
  "rssi": -47,
  "timestamp": "12:45 28/10"
}
```

#### 🔹 Ejemplo de comando MQTT hacia nodo LoRa:
Enviado al tópico:
```
gateway/Gat_01/lora/cmd
```
Payload:
```json
{
  "node_id": "NODE_A",
  "action": "RESET"
}
```

El gateway retransmite la orden por LoRa:
```
<LoRa TX> NODE_A:RESET
```

---

## 🔧 Hardware Requerido

| Componente | Descripción |
|-------------|-------------|
| **ESP32 DOIT DevKit V1** | Microcontrolador principal |
| **Módulo LoRa SX1276 / SX1278** | Comunicación inalámbrica entre nodos |
| **LCD I2C 16x2** | Visualización de estado |
| **Botón pulsador (NO)** | Reset WiFi físico |
| **Fuente 5V** | Alimentación estable |
| **Cable USB** | Programación y depuración |

**Conexiones principales:**

| Componente | ESP32 Pin | Notas |
|-------------|-----------|-------|
| LCD SDA | GPIO 21 | I2C Data |
| LCD SCL | GPIO 22 | I2C Clock |
| LoRa MOSI | GPIO 23 | SPI |
| LoRa MISO | GPIO 19 | SPI |
| LoRa SCK | GPIO 18 | SPI |
| LoRa NSS | GPIO 5 | Chip select |
| LoRa DIO0 | GPIO 26 | Interrupción RX |
| Botón | GPIO 33 | Con `INPUT_PULLDOWN` |

---

## 🧪 Intervalos y comportamiento

| Acción | Intervalo |
|--------|------------|
| Publicación MQTT | 30 s |
| Ping MQTT | 30 s |
| Actualización LCD | 2 s |
| Reintento WiFi | 10 s |
| Reconexión WS/MQTT | 5 s |
| Duración modo AP | 3 min |
| Hold botón reset | 3 s |

---

## 🛠️ Solución de Problemas

| Problema | Posible causa | Solución |
|-----------|----------------|-----------|
| LCD sin texto | Dirección I2C incorrecta | Cambiar 0x27 ↔ 0x3F |
| No se conecta WiFi | SSID/clave incorrecta o 5 GHz | Revisar credenciales / usar 2.4 GHz |
| MQTT no conecta | Broker no accesible o puerto cerrado | Verificar `mqtt.ispciot.org:80` |
| LoRa no recibe | Pines o frecuencia incorrecta | Verificar wiring y frecuencia LoRa |
| Botón no responde | GPIO inválido | Usar GPIO 33 (no GPIO 34–39) |

---

## 📄 Licencia

Este proyecto se distribuye bajo la **Licencia MIT**.  
Puedes usarlo, modificarlo y redistribuirlo libremente, siempre que se mantenga el aviso de autoría original.

---

## 🙏 Agradecimientos

- Biblioteca **WebSockets** de [Links2004](https://github.com/Links2004/arduinoWebSockets)  
- Biblioteca **LiquidCrystal_I2C** de [marcoschwartz](https://github.com/marcoschwartz/LiquidCrystal_I2C)  
- Biblioteca **ArduinoJson** de [bblanchon](https://github.com/bblanchon/ArduinoJson)  
- Comunidad de desarrolladores ESP32  
- Documentación oficial del protocolo **MQTT 3.1.1**

---

**Proyecto:** Embebido silo  
**Versión:** 1.2    
**Última actualización:** Octubre 2025  
**Plataforma:** ESP32 (espressif32)  
**Framework:** Arduino  

---

Nota: Este proyecto fue desarrollado como prueba unitaria para sistemas IoT con MQTT sobre WebSocket, Para la instancia intertecnicatura del ISPC en conjunto de TS-telecomunicaciones y TS-Desarrollo de software
