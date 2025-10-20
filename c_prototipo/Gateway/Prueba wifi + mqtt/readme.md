# Gateway MQTT con ESP32

Sistema de gateway MQTT sobre WebSocket para ESP32, con configuración WiFi mediante portal web, visualización de estado en pantalla LCD y botón de reset físico.

## 📋 Características

- ✅ Conexión MQTT sobre WebSocket (puerto 80)
- ✅ Portal web de configuración WiFi (modo Access Point)
- ✅ Pantalla LCD I2C para visualización de estado
- ✅ Reconexión automática WiFi y MQTT
- ✅ Publicación periódica de estado del gateway
- ✅ Sincronización horaria con NTP (zona horaria Argentina)
- ✅ Botón de reset físico para borrar configuración WiFi
- ✅ Sistema anti-parpadeo en LCD

## 🔧 Hardware Requerido

- **ESP32 DOIT DevKit V1** (o compatible)
- **Pantalla LCD I2C 16x2** (dirección 0x27 o 0x3F)
- **Botón pulsador** (Normally Open)
- Cable USB para programación
- Fuente de alimentación 5V

### Conexiones

| Componente | ESP32 Pin | Notas |
|------------|-----------|-------|
| LCD VCC | 3.3V | Alimentación |
| LCD GND | GND | Tierra |
| LCD SDA | GPIO 21 | I2C Data |
| LCD SCL | GPIO 22 | I2C Clock |
| Botón Terminal 1 | GPIO 33 | Con pull-down interno |
| Botón Terminal 2 | 3.3V | Activación |

**Diagrama del botón:**
```
3.3V ───┬─── Botón ───┬─── GPIO 33
        │             │
        └─────────────┘
```

## 📦 Dependencias

Las siguientes librerías se instalan automáticamente con PlatformIO:

```ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
lib_deps =
  bblanchon/ArduinoJson@^7.4.2
  marcoschwartz/LiquidCrystal_I2C@^1.1.4
  links2004/WebSockets@^2.7.1
monitor_speed = 115200
```

**Nota:** Este proyecto usa implementación manual de MQTT (no requiere PubSubClient).

## 🚀 Instalación

### 1. Clonar el repositorio

```bash
git clone <url-repositorio>
cd gateway_mqtt
```

### 2. Compilar y cargar

Con PlatformIO CLI:

```bash
pio run --target upload
```

Con PlatformIO IDE:
- Abrir el proyecto en VSCode
- Hacer clic en el botón "Upload" (→) en la barra inferior

### 3. Monitor Serial (opcional)

```bash
pio device monitor
```

Velocidad: **115200 baudios**

## 📱 Configuración WiFi (Primera vez)

### Paso 1: Modo Access Point
Cuando el ESP32 no tiene WiFi configurado:
- Crea un punto de acceso llamado: `GatewayMQTT_Gat_01`
- El modo AP permanece activo por **3 minutos**
- La pantalla LCD muestra:
  ```
  Modo AP Activo
  IP: 192.168.4.1
  ```

### Paso 2: Conectarse al AP
1. Desde tu celular o PC, busca la red WiFi `GatewayMQTT_Gat_01`
2. Conéctate (no requiere contraseña)

### Paso 3: Configurar WiFi
1. Abre el navegador web
2. Navega a: `http://192.168.4.1`
3. Ingresa tu **SSID** (nombre de red WiFi)
4. Ingresa tu **contraseña** WiFi
5. Haz clic en "Guardar y Conectar"

### Paso 4: Reinicio automático
- El ESP32 guarda la configuración en memoria permanente (NVS)
- Se reinicia automáticamente
- Se conecta al WiFi configurado
- Inicia conexión MQTT

## 🔄 Reset de Configuración WiFi

### Botón de Reset (GPIO 33)
Para borrar la configuración WiFi guardada:

1. **Mantener presionado** el botón por **3 segundos**
2. El LCD muestra la cuenta regresiva:
   ```
   Reset WiFi: 1/3
   ████────────────
   
   Reset WiFi: 2/3
   ████████────────
   
   Reset WiFi: 3/3
   ████████████████
   ```
3. Al completar 3 segundos:
   ```
   Borrando WiFi
   Reiniciando...
   ```
4. El dispositivo se reinicia en modo AP

### Cancelar Reset
Si sueltas el botón **antes de 3 segundos**:
```
Cancelado
```
El dispositivo vuelve a operación normal sin borrar la configuración.

## 🌐 Funcionamiento Normal

### Conexión WiFi
- El ESP32 se conecta automáticamente al WiFi guardado
- Si pierde conexión, reintenta cada **10 segundos**
- La IP asignada se muestra en el monitor serial

### Conexión MQTT
- **Servidor:** `mqtt.ispciot.org`
- **Puerto:** 80 (WebSocket)
- **Path:** `/mqtt`
- **Client ID:** `Gat_01`
- **Protocolo:** MQTT 3.1.1 sobre WebSocket
- Keepalive: **60 segundos**
- Ping cada **30 segundos**
- Reconexión automática cada **5 segundos** si se pierde

### Publicación de Estado
Cada **30 segundos**, el gateway publica en:

**Tópico:** `gateway/Gat_01/status`

**Formato JSON:**
```json
{
  "gateway_id": "Gat_01",
  "rssi": -45,
  "quality": "Excelente",
  "timestamp": "14:30 19/10"
}
```

### Calidad de Señal WiFi
- **Excelente:** RSSI ≥ -50 dBm
- **Buena:** RSSI ≥ -60 dBm
- **Regular:** RSSI ≥ -70 dBm
- **Débil:** RSSI < -70 dBm

### Suscripción MQTT
El gateway se suscribe automáticamente al tópico:
- `gateway/Gat_01/cmd` (para recibir comandos)

## 📺 Pantalla LCD

La pantalla LCD muestra información en tiempo real con sistema anti-parpadeo (actualización cada 2 segundos, solo si cambia el contenido):

### Secuencia de Inicio
```
Gateway MQTT        ← Splash inicial (2s)
Iniciando...

WiFi Config: SI     ← Si hay configuración
Conectando...

Conectando WiFi     ← Intentando conexión
...

WiFi: OK           ← Conexión exitosa
Init MQTT...

MQTT: Esperando    ← Estableciendo MQTT
Conexion...
```

### Modo AP
```
Modo AP Activo
IP: 192.168.4.1
```

### Operación Normal - WiFi Conectado
```
WiFi: Conectado
MQTT: Conectado
```

### Operación Normal - MQTT Desconectado
```
WiFi: Conectado
MQTT: Esperando...
```

### WiFi Desconectado
```
WiFi: Buscando
Red...
```

### Reset de Configuración
```
Mantener 3 seg      ← Al presionar botón
para Reset WiFi

Reset WiFi: 2/3     ← Manteniendo presionado
████████--------

Borrando WiFi       ← Al completar 3s
Reiniciando...

Cancelado           ← Si se suelta antes
```

### Sin Configuración (después de timeout AP)
```
AP Timeout
Sin config WiFi
```

## 📊 Intervalos de Tiempo

| Acción | Intervalo |
|--------|-----------|
| Publicación MQTT | 30 segundos |
| MQTT Ping | 30 segundos |
| Actualización LCD | 2 segundos |
| Reintentos WiFi | 10 segundos |
| Reconexión WebSocket | 5 segundos |
| Duración modo AP | 3 minutos |
| Hold tiempo botón reset | 3 segundos |

## 📁 Estructura del Proyecto

```
gateway_mqtt/
├── src/
│   ├── main.cpp              # Loop principal y setup
│   ├── conexiones.h          # Declaraciones WiFi/MQTT/WebSocket
│   ├── conexiones.cpp        # Implementación conexiones y reset
│   ├── lcdplus.h            # Declaraciones LCD con anti-flicker
│   ├── lcdplus.cpp          # Implementación LCD y caché
│   ├── web_portal.h         # Declaraciones portal web
│   ├── web_portal.cpp       # Implementación servidor AP
│   ├── mqtt_manual.h        # Cliente MQTT manual sobre WS
│   └── WebSocketClient.h    # Wrapper WebSocket-Cliente (opcional)
├── platformio.ini           # Configuración PlatformIO
└── README.md               # Este archivo
```

## ⚙️ Personalización

### Cambiar ID del Gateway
En `conexiones.cpp`, línea 7:
```cpp
const char* gatewayId = "Gat_01";  // Cambiar aquí
```
**Nota:** Esto también cambia el SSID del AP y los tópicos MQTT.

### Cambiar Servidor MQTT
En `conexiones.cpp`, líneas 8-10:
```cpp
const char* mqtt_server = "mqtt.ispciot.org";
const uint16_t mqtt_port = 80;
const char* mqtt_path = "/mqtt";
```

### Cambiar Pin del Botón Reset
En `conexiones.cpp`, línea 14:
```cpp
const int RESET_BUTTON_PIN = 33;  // Usar GPIO con pull-up/down interno
```
**⚠️ Importante:** No usar GPIO 34, 35, 36, 39 (no tienen pull-up/down interno).

### Cambiar Dirección LCD I2C
En `lcdplus.cpp`, línea 7:
```cpp
static LiquidCrystal_I2C lcd(0x27, 16, 2);  // Probar 0x3F si no funciona
```

### Ajustar Intervalos
En `main.cpp`, líneas 7-10:
```cpp
const unsigned long apDuration = 180000;       // 3 minutos AP
const unsigned long publishInterval = 30000;   // 30s publicación
const unsigned long lcdInterval = 2000;        // 2s actualización LCD
```

En `conexiones.cpp`, línea 24:
```cpp
const unsigned long pingInterval = 30000;  // 30s MQTT ping
```

### Cambiar Tiempo Hold Botón Reset
En `conexiones.cpp`, función `checkResetButton()`:
```cpp
const unsigned long HOLD_TIME = 3000;  // 3 segundos
```

## 🛠 Solución de Problemas

### LCD no muestra nada
- ✓ Verificar conexiones I2C (SDA/SCL en GPIO 21/22)
- ✓ Probar dirección alternativa: cambiar `0x27` por `0x3F` en `lcdplus.cpp`
- ✓ Ajustar contraste del LCD (potenciómetro en el módulo I2C)
- ✓ Verificar alimentación 3.3V/5V según especificaciones del LCD

### LCD muestra texto mezclado o parpadea
- ✓ Actualizar a la versión mejorada de `lcdplus.cpp` con sistema de caché
- ✓ Verificar que `lcdInterval` no sea menor a 1000ms
- ✓ No llamar `getLCD().print()` directamente desde otras funciones

### No se conecta al WiFi
- ✓ Verificar SSID y contraseña correctos (case-sensitive)
- ✓ Asegurarse de que el router está en 2.4GHz (ESP32 no soporta 5GHz)
- ✓ Revisar monitor serial para mensajes de error
- ✓ Verificar que el router no tiene filtrado MAC activo
- ✓ Intentar reset de configuración WiFi con el botón

### No se conecta a MQTT
- ✓ Verificar que el servidor `mqtt.ispciot.org` está accesible
- ✓ Comprobar que el puerto 80 está abierto
- ✓ Revisar monitor serial: buscar "✅ MQTT Conectado!" o errores
- ✓ Verificar conectividad WiFi primero
- ✓ Intentar ping manual al servidor desde tu red

### El modo AP no aparece
- ✓ Usar el botón de reset (mantener 3 segundos)
- ✓ Verificar que el botón está conectado entre GPIO 33 y 3.3V
- ✓ Revisar monitor serial para confirmar que se borró la configuración

### Botón de reset no responde
- ✓ Verificar conexión: Terminal 1 → GPIO 33, Terminal 2 → 3.3V
- ✓ Probar invertir terminales del botón
- ✓ Verificar que el código usa `INPUT_PULLDOWN` (no `INPUT_PULLUP`)
- ✓ Comprobar continuidad del botón con multímetro
- ✓ Asegurarse de usar GPIO 33 (NO GPIO 34)

### Botón se activa solo o comportamiento errático
- ✓ Verificar que NO estás usando GPIO 34, 35, 36 o 39
- ✓ Confirmar que el código lee `HIGH` para detectar presión
- ✓ Revisar que el botón no está en corto permanente
- ✓ Usar cable corto para el botón (evitar cables largos > 20cm)

### WebSocket se desconecta frecuentemente
- ✓ Verificar estabilidad de la conexión WiFi (RSSI > -70 dBm)
- ✓ Revisar que el router no tiene configuraciones agresivas de ahorro de energía
- ✓ Confirmar que el servidor MQTT acepta conexiones WebSocket en puerto 80
- ✓ Verificar logs del servidor MQTT

### Memoria Flash casi llena (>90%)
- ✓ Reducir tamaño de buffers en `mqtt_manual.h` (línea 19: `uint8_t buf[512]`)
- ✓ Eliminar mensajes Serial innecesarios
- ✓ Considerar particiones personalizadas en `platformio.ini`

## 📊 Uso de Recursos

- **RAM:** ~15% (48,372 bytes / 327,680 bytes)
- **Flash:** ~75% (978,449 bytes / 1,310,720 bytes)

**Componentes principales:**
- Cliente MQTT manual: ~5KB
- WebSocket: ~20KB
- LCD con caché: ~2KB
- Portal web: ~3KB

## 🔍 Monitoreo y Debugging

### Monitor Serial (115200 baud)

**Secuencia típica de inicio exitoso:**
```
=== Gateway MQTT ESP32 ===

LCD inicializado
Conectando a WiFi: MiRed
.....
✅ WiFi conectado!
IP: 192.168.1.100
MQTT/WebSocket inicializado
Esperando WebSocket...
✅ WebSocket conectado
📤 MQTT CONNECT enviado
✅ MQTT Conectado!
📤 Suscrito a: gateway/Gat_01/cmd
📤 Publicado [gateway/Gat_01/status]: {"gateway_id":"Gat_01"...}
```

**Eventos de reset de configuración:**
```
Mantener 3 seg
✅ Configuración WiFi borrada
```

**Eventos MQTT:**
```
📤 MQTT CONNECT enviado
✅ MQTT Conectado!
📤 Suscrito a: gateway/Gat_01/cmd
📤 Publicado [gateway/Gat_01/status]: {...}
📥 Recibido [gateway/Gat_01/cmd]: {...}
🏓 PING
🏓 PONG
```

## 🔐 Seguridad

⚠️ **Consideraciones importantes:**

**Limitaciones actuales:**
- El portal web no usa HTTPS (limitación del ESP32)
- Las credenciales WiFi se guardan en texto plano en NVS
- El modo AP no tiene contraseña (solo activo 3 minutos)
- No hay autenticación MQTT

**Para entornos de producción, considerar:**
- ✓ Contraseña en el modo AP
- ✓ Cifrado de credenciales en NVS
- ✓ Autenticación MQTT con usuario/contraseña
- ✓ Certificados TLS/SSL (requiere puerto 8883)
- ✓ Timeout más corto del modo AP (< 3 min)
- ✓ Rate limiting en el portal web

## 🧪 Pruebas Unitarias Implementadas

Este proyecto sirve como prueba unitaria de los siguientes componentes:

- ✅ Portal cautivo de configuración WiFi
- ✅ Persistencia de credenciales (NVS/Preferences)
- ✅ Reconexión automática WiFi
- ✅ Cliente MQTT manual sobre WebSocket
- ✅ Publicación periódica de mensajes MQTT
- ✅ Suscripción y recepción de mensajes MQTT
- ✅ Keepalive MQTT (PING/PONG)
- ✅ Display LCD con sistema anti-parpadeo
- ✅ Manejo de estados temporales en LCD
- ✅ Reset físico con botón y feedback visual
- ✅ Sincronización NTP con zona horaria
- ✅ Timeout de Access Point

## 📝 Notas Técnicas

### MQTT sobre WebSocket
- Implementación manual del protocolo MQTT 3.1.1
- No requiere biblioteca PubSubClient
- Envía paquetes binarios MQTT dentro de frames WebSocket
- Soporta QoS 0 (sin confirmación de entrega)

### Sistema Anti-Parpadeo LCD
- Caché de líneas previas (`lastLine1`, `lastLine2`)
- Solo actualiza cuando el contenido cambia
- Reduce parpadeo y mejora legibilidad
- Respeta mensajes temporales con prioridad

### GPIO y Pull Resistors
- **GPIO 33:** Tiene pull-up/pull-down interno (✅ recomendado para botones)
- **GPIO 34-39:** NO tienen pull-up/pull-down interno (❌ evitar para botones)
- El código usa `INPUT_PULLDOWN` para lógica natural (LOW = reposo, HIGH = activo)

### Almacenamiento NVS
- Las credenciales se guardan en partición NVS (Non-Volatile Storage)
- Sobrevive a reinicios y cortes de energía
- Se puede borrar con el botón de reset o reprogramación

## 📄 Licencia

[Especificar tu licencia aquí - Ejemplo: MIT, GPL, Apache, etc.]

## 👥 Contribuciones

Las contribuciones son bienvenidas. Por favor:

1. Fork del repositorio
2. Crear rama para tu feature (`git checkout -b feature/NuevaFuncionalidad`)
3. Commit de cambios (`git commit -m 'Agregar nueva funcionalidad'`)
4. Push a la rama (`git push origin feature/NuevaFuncionalidad`)
5. Abrir Pull Request

**Guías de contribución:**
- Mantener el estilo de código consistente
- Documentar funciones nuevas
- Probar en hardware real antes de PR
- Actualizar README si es necesario

## 🙏 Agradecimientos

- Biblioteca **WebSockets** de [Links2004](https://github.com/Links2004/arduinoWebSockets)
- Biblioteca **LiquidCrystal_I2C** de [marcoschwartz](https://github.com/marcoschwartz/LiquidCrystal_I2C)
- Biblioteca **ArduinoJson** de [bblanchon](https://github.com/bblanchon/ArduinoJson)
- Comunidad de PlatformIO y ESP32
- Documentación de MQTT Protocol v3.1.1

## 📧 Contacto

[Tu información de contacto - Ejemplo: email, GitHub, Discord, etc.]

---

**Proyecto:** Gateway MQTT ESP32 - Prueba Unitaria  
**Versión:** 2 
**Última actualización:** Octubre 2025  
**Plataforma:** ESP32 (espressif32)  
**Framework:** Arduino

---

**Nota:** Este proyecto fue desarrollado como prueba unitaria para sistemas IoT con MQTT sobre WebSocket.
