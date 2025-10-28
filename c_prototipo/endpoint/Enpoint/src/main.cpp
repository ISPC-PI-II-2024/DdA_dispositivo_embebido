#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#define ENDPOINT_ID "01"

// Pines LoRa (ESP32-C3)
#define LORA_SCK 4
#define LORA_MISO 5
#define LORA_MOSI 6
#define LORA_CS 7
#define LORA_RST -1
#define LORA_DIO0 3

// LEDs
#define LED_ROJO 8
#define LED_AMARILLO 9
#define LED_VERDE 10

#define LORA_FREQ 433E6
#define INTERVALO_ENVIO 15000      // 15 segundos
#define LORA_CHECK_INTERVAL 10000  // 10 segundos

unsigned long ultimoEnvio = 0;
unsigned long Ultimo_Check_Lora = 0;
bool lora_Conectado = false;

uint8_t readLoRaVersionSPI()
{
  uint8_t version;
  digitalWrite(LORA_CS, LOW);
  SPI.transfer(0x42 & 0x7F);
  version = SPI.transfer(0x00);
  digitalWrite(LORA_CS, HIGH);
  return version;
}

bool iniciar_Lora()
{
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) return false;

  uint8_t version = readLoRaVersionSPI();
  if (version != 0x12 && version != 0x13) {
    LoRa.end();
    return false;
  }

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();

  Serial.println("LoRa OK");
  return true;
}

void reconnectLora()
{
  lora_Conectado = false;
  digitalWrite(LED_VERDE, LOW);
  LoRa.end();
  delay(200);

  while (!lora_Conectado) {
    digitalWrite(LED_ROJO, HIGH);
    delay(100);
    digitalWrite(LED_ROJO, LOW);
    delay(200);
    if (iniciar_Lora()) {
      lora_Conectado = true;
      digitalWrite(LED_VERDE, HIGH);
    }
  }
}

void enviarMensaje()
{
  String mensaje = "Hola desde ENDPOINT - ";
  mensaje += ENDPOINT_ID;

  digitalWrite(LED_AMARILLO, HIGH);
  LoRa.beginPacket();
  LoRa.print(mensaje);
  LoRa.endPacket();
  digitalWrite(LED_AMARILLO, LOW);

  Serial.println("Enviado: " + mensaje);
}

void setup()
{
  Serial.begin(115200);
  delay(500);
  Serial.println("Iniciando endpoint...");

  pinMode(LED_ROJO, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_VERDE, LOW);

  // Secuencia de inicio
  digitalWrite(LED_ROJO, HIGH);
  delay(500);
  digitalWrite(LED_ROJO, LOW);

  lora_Conectado = iniciar_Lora();
  if (!lora_Conectado) {
    reconnectLora();
  } else {
    digitalWrite(LED_VERDE, HIGH);
  }

  ultimoEnvio = millis();
  Ultimo_Check_Lora = millis();

  // Primer envío inmediato
  enviarMensaje();
}

void loop()
{
  unsigned long currentMillis = millis();

  // Verificar conexión LoRa cada 10s
  if (currentMillis - Ultimo_Check_Lora >= LORA_CHECK_INTERVAL) {
    Ultimo_Check_Lora = currentMillis;
    uint8_t version = readLoRaVersionSPI();
    if (version != 0x12 && version != 0x13) {
      Serial.println("Error: chip LoRa no responde");
      reconnectLora();
    }
  }

  if (!lora_Conectado) return;

  // ✅ Enviar cada 15 segundos
  if (currentMillis - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = currentMillis; // Actualizar ANTES de enviar
    enviarMensaje();
  }

  // Escuchar ACKs del gateway
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String msg = "";
    while (LoRa.available()) {
      msg += (char)LoRa.read();
    }
    msg.trim();
    Serial.println("Recibido: " + msg);

    String expectedAck = "ACK:" + String(ENDPOINT_ID);
    if (msg == expectedAck) {
      digitalWrite(LED_ROJO, HIGH);
      delay(500); // Breve confirmación
      digitalWrite(LED_ROJO, LOW);
    }
  }

  delay(5); // Pequeño delay para estabilidad
}