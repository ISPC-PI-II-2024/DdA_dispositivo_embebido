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
| **Endpoint**        | ESP32 + LoRa + múltiples sensores AHT10 + RS485 | Captura de temperatura y humedad desde múltiples sensores. RS485 permite extender la distancia entre sensores y el microcontrolador. |
| **Gateway**         | ESP32 + LoRa + GSM + Pantalla LCD 40x2 I2C       | Recibe datos vía LoRa, los transmite en tiempo real vía MQTT a un broker en servidor dedicado. La pantalla está disponible para mostrar información como señal, batería y cantidad de endpoints conectados. |

---

## 📡 Comunicación

- **Entre nodos**: LoRa (Gateway ↔ Endpoint)  
- **Transmisión remota**: MQTT sobre GSM (Gateway → Broker dedicado)  
- **Sensores**: AHT10 (temperatura y humedad)  
- **Extensión de sensores**: RS485 para mayor alcance físico  

---

## 📂 Estructura del repositorio
| Carpeta | Contenido |
|--------|---------|
| `a_requisitos/` | Definición del problema, objetivos y funcionalidades |
| `b_investigacion/` | Fundamentos técnicos, protocolos y arquitectura |
| `c_prototipo/` | Código fuente del nodo y gateway  |
| `d_presentacion/` | Presentación final, guion y reflexión |
| `assets/` | Imágenes, diagramas y recursos multimedia |

## 🛠️ Tecnologías utilizadas

- Microcontroladores: ESP32  
- Sensores: AHT10  
- Pantalla: LCD 40x2 con interfaz I2C  
- Comunicación: LoRa, RS485, GSM  
- Protocolo de transmisión: MQTT  
- Entorno de desarrollo: Visual Studio Code + PlatformIO  

---


---

## 📊 Estado del proyecto

- [x] Definición de arquitectura  
- [ ] Programación de firmware  
- [ ] Comunicación LoRa entre nodos  
- [ ] Visualización en pantalla  
- [ ] Transmisión MQTT al broker  
- [ ] Validación en entorno de silo  

---

## 📎 Enlaces útiles
- [Definir los enlaces a la informacion correcta (Solicitado por el profesor para mejor legibilidad)] (pendiente)
- [Guía de contribución](CONTRIBUTING.md)
