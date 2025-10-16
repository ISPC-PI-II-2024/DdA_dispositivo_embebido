#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <SoftwareSerial.h>

// ==== Configuración de pines ====
#define SDA_PIN 2
#define SCL_PIN 1
#define RX_PIN  7  // RO del MAX485
#define TX_PIN  6  // DI del MAX485
#define DE_RE_PIN 5 // DE y RE juntos

Adafruit_AHTX0 aht;
SoftwareSerial RS485(RX_PIN, TX_PIN); // RX, TX

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Iniciando sistema RS485 + AHT10...");

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!aht.begin()) {
    Serial.println("No se detecta el AHT10. Verifica conexiones SDA/SCL.");
    while (1);
  }
  Serial.println("AHT10 detectado correctamente.");

  // RS485
  RS485.begin(9600);
  pinMode(DE_RE_PIN, OUTPUT);
  digitalWrite(DE_RE_PIN, LOW); // recepción por defecto
  Serial.println("RS485 inicializado correctamente.");

  Serial.println("Sistema listo para enviar lecturas cada 5s.\n");
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  // Construimos mensaje con ID de nodo
  String mensaje = "N1|Tempreatura: " + String(temp.temperature, 1) + "°C|Humedad:" + String(humidity.relative_humidity, 1) + "%";

  // Transmitimos por RS485
  digitalWrite(DE_RE_PIN, HIGH);  // habilita transmisión
  RS485.println(mensaje);
  RS485.flush(); // espera a que termine
  digitalWrite(DE_RE_PIN, LOW);   // vuelve a recepción

  // Log local por USB
  Serial.println("Enviado por RS485" + mensaje);

  delay(5000);
}
