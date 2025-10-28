# 🛠️ Solución de Problemas

Guía completa de troubleshooting para el sistema IoT multinivel.

## 📑 Índice

- [Gateway](#gateway)
- [Endpoint](#endpoint)
- [Nodo Sensor](#nodo-sensor)
- [Comunicación LoRa](#comunicación-lora)
- [Comunicación RS485](#comunicación-rs485)
- [Conectividad](#conectividad)

---

## Gateway

### LCD no muestra nada

**Síntomas:**
- Pantalla LCD apagada o sin texto
- Backlight encendido pero sin caracteres

**Soluciones:**

1. **Verificar dirección I2C**
   ```bash
   # Usar escáner I2C
   pio run -t upload -e i2c_scanner
   ```
   - Probar `0x27` o `0x3F` en `lcdplus.cpp`

2. **Verificar conexiones**
   - SDA → GPIO 21
   - SCL → GPIO 22
   - VCC → 3.3V (algunos LCD necesitan 5V)
   - GND → GND

3. **Ajustar contraste**
   - Girar potenciómetro azul en módulo I2C
   - Debe verse texto al ajustar

4. **Verificar voltaje**
   ```cpp
   // Si LCD necesita 5V, usar Vin en lugar de 3.3V
   ```

### LCD muestra texto mezclado o parpadea

**Síntomas:**
- Caracteres superpuestos
- Parpadeo constante
- Texto ilegible

**Soluciones:**

1. **Actualizar a versión con caché**
   - Usar artifact "lcdplus.cpp - Mejorado"
   - El sistema de caché evita actualizaciones innecesarias

2. **Reducir frecuencia de actualización**
   ```cpp
   // main.cpp
   const unsigned long lcdInterval = 3000; // Aumentar a 3s
   ```

3. **No escribir directamente al LCD**
   ```cpp
   // ❌ INCORRECTO
   getLCD().print("Texto");
   
   // ✅ CORRECTO
   mostrarMensajeLCD("Línea 1", "Línea 2", 0);
   ```

### WiFi no conecta

**Síntomas:**
- Gateway se queda en "Conectando WiFi..."
- No obtiene IP

**Soluciones:**

1. **Verificar credenciales**
   - SSID es case-sensitive
   - Contraseña correcta
   - Red 2.4GHz (ESP32 no soporta 5GHz)

2. **Reset configuración**
   - Mantener botón GPIO 33 por 3 segundos
   - Reintentar configuración

3. **Verificar router**
   - Sin filtrado MAC
   - DHCP habilitado
   - Sin aislamiento AP

4. **Monitor serial**
   ```bash
   pio device monitor -b 115200
   # Ver mensajes de error específicos
   ```

### MQTT no conecta

**Síntomas:**
- WiFi OK pero "MQTT: Esperando..."
- No publica mensajes

**Soluciones:**

1. **Verificar conectividad al broker**
   ```bash
   ping mqtt.ispciot.org
   ```

2. **Verificar puerto**
   - Puerto 80 debe estar abierto
   - Algunos routers/ISP bloquean puerto 80

3. **Verificar WebSocket**
   ```cpp
   // conexiones.cpp - verificar configuración
   const char* mqtt_server = "mqtt.ispciot.org";
   const uint16_t mqtt_port = 80;
   const char* mqtt_path = "/mqtt";
   ```

4. **Monitor serial - buscar errores**
   ```
   ✅ WebSocket conectado
   📤 MQTT CONNECT enviado
   ✅ MQTT Conectado!
   ```

### LoRa no inicializa

**Síntomas:**
- LCD muestra "LoRa: ERROR"
- Serial: "❌ LoRa: Error al inicializar"

**Soluciones:**

1. **Verificar conexiones SPI**
   | Pin | GPIO |
   |-----|------|
   | SCK | 18 |
   | MISO | 19 |
   | MOSI | 23 |
   | CS | 5 |
   | RST | 26 |
   | DIO0 | 27 |

2. **Verificar alimentación**
   - Módulo LoRa necesita 3.3V estable
   - Corriente mínima: 100mA

3. **Probar comunicación SPI**
   ```cpp
   // Verificar versión del chip
   uint8_t version = readLoRaVersionSPI();
   Serial.printf("Versión LoRa: 0x%02X\n", version);
   // Debe ser 0x12 o 0x13
   ```

4. **Verificar módulo**
   - Chip debe ser SX1276/77/78
   - Frecuencia correcta (433MHz o 915MHz)

### Botón reset no responde

**Síntomas:**
- Presionar botón no hace nada
- No aparece mensaje en LCD

**Soluciones:**

1. **Verificar conexión**
   - Terminal 1 → GPIO 33
   - Terminal 2 → 3.3V
   - Probar invertir terminales

2. **Verificar código**
   ```cpp
   // conexiones.cpp - debe ser INPUT_PULLDOWN
   pinMode(RESET_BUTTON_PIN, INPUT_PULLDOWN);
   
   // Leer como HIGH cuando se presiona
   bool buttonState = digitalRead(RESET_BUTTON_PIN) == HIGH;
   ```

3. **Probar continuidad**
   - Usar multímetro
   - Verificar que el botón cierra circuito

4. **No usar GPIO 34**
   - GPIO 34 no tiene pull-down interno
   - Usar GPIO 33 como en el código

---

## Endpoint

### 