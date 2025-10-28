# 🌾 Sistema Embebido para Monitoreo de Silos – Proyecto Intertecnicatura

## 🎯 Propósito

Este proyecto tiene como objetivo el desarrollo de un sistema embebido distribuido para el **monitoreo ambiental de silos metálicos verticales**, en el marco de una **demostración educativa interdisciplinaria** entre las tecnicaturas de:

- Telecomunicaciones  
- Desarrollo de Software  
- Tecnologías Aplicadas al Agro

---


## 👥 Equipo
| Nombre                        | GitHub                                 |
|------------------------------|----------------------------------------|
| Leandro Roldan               | [@pleroldan](https://github.com/pleroldan) |
| Gaston Osess               | [@cholobackcod](https://github.com/cholobackcod) |
| Raul Jara              | [@r-j28](https://github.com/r-j28) |
| Lautaro Villafañe               | [@lautiiv](https://github.com/lautiiv) |

## 🧱 Arquitectura del sistema

| Rol del dispositivo | Componentes | Función principal |
|---------------------|-------------|-------------------|
| **Gateway**         | ESP32 + LoRa + GSM + Pantalla LCD 40x2 I2C       | Recibe datos vía LoRa, los transmite en tiempo real vía MQTT a un broker en servidor dedicado. La pantalla está disponible para mostrar información como señal, batería y cantidad de endpoints conectados. |
| **Endpoint**        | ESP32 + LoRa + RS485 | Recolección de datos en los distintos sensores,  RS485 permite intenconectar multiples dispositivos hacia el endpoint. |
|** Micro dedicado, conectado al sensor ** | ESP8266 + sensor AHT10 + RS485 | Sensorizacion del entorno fisico (Se utiliza un mcu para lograr colocar el sensor alejado del endpoint) |
---

## 📡 Comunicación
- **Transmisión remota**: MQTT sobre GSM (Gateway → Broker dedicado)  
- **Entre nodos**: LoRa (Gateway ↔ Endpoint)  

- **Sensores**: AHT10 (temperatura y humedad)  i2c
- **Extensión de sensores**: RS485 para mayor alcance físico  entre endpoint y micro dedicado

---

## 📂 Estructura del repositorio
| Carpeta | Contenido |
|--------|---------|
| `a_requisitos/` | Definición del problema, objetivos y funcionalidades |
| `b_investigacion/` | Fundamentos técnicos, protocolos y arquitectura |
| `c_prototipo/` | Códigos fuentes del nodo, gateway y sensores |
| `d_presentacion/` | Presentación final, guion y reflexión |
| `assets/` | Imágenes, diagramas y recursos multimedia |

## 🛠️ Tecnologías utilizadas

- Microcontroladores: ESP32  ESP8266
- Sensores: AHT10  
- Pantalla: LCD 40x2 con interfaz I2C  
- Comunicación: LoRa, RS485, GSM, I2C
- Protocolo de transmisión: MQTT  
- Entorno de desarrollo: Visual Studio Code + PlatformIO  

---


---

## 📊 Estado del proyecto

- [x] Definición de arquitectura  
- [ ] Programación de firmware Gateway  ---80%
- [ ] Programación de firmware Endpoint ---60%
- [x] Comunicación LoRa entre nodos  
- [x] Visualización en pantalla  
- [x] Transmisión MQTT al broker  
- [ ] Validación en entorno de silo  

---

## 📎 Enlaces útiles
- [Definir los enlaces a la informacion correcta (Solicitado por el profesor para mejor legibilidad)] (pendiente)

