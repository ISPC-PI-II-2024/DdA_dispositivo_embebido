# 2. Protocolos de comunicación IoT

## 📡 MQTT
- Ligero, ideal para redes con bajo ancho de banda.
- Basado en pub/sub.
- Usado en este proyecto para enviar datos al frontend.

## 📶 LoRa / LoRaWAN
- Larga distancia, baja potencia.
- Ideal para sensores remotos.
- Simulado en el frontend (no requiere hardware real).

## 🌐 HTTP/REST
- Simple, pero consume más recursos.
- Usado para configuraciones estáticas.

## 🔐 Seguridad
- SSL/TLS para conexiones seguras.
- Autenticación por tokens (si aplica).
