#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <LoRa.h>

// ⚠️ Asegúrate de usar la dirección I2C correcta (0x27 o 0x3F)
LiquidCrystal_I2C lcd(0x27, 20, 4);  // ← AJUSTA SEGÚN TU ESCÁNER

#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST 26
#define LORA_DIO0 27
#define LORA_FREQ 433E6

unsigned long contactosTotales = 0;
unsigned long tiempoPantallaMensaje = 0;
bool mostrarMensajeTemporal = false;

// Función para clasificar la calidad del SNR
String calidadSenal(float snr) {
  if (snr > 10.0) return "Excelente";
  else if (snr >= 0.0) return "Buena    "; // Espacios para limpiar
  else return "Debil    ";
}

bool iniciar_Lora()
{
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQ)) return false;

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  return true;
}

String extraerID(const String& msg)
{
  int pos = msg.indexOf("ENDPOINT - ");
  if (pos == -1) return "";
  return msg.substring(pos + 11);
}

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando LoRa...");

  if (!iniciar_Lora()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error LoRa");
    while (1) delay(1000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LoRa OK");
  lcd.setCursor(0, 1);
  lcd.print("Esperando datos...");
  delay(1500);
}

void loop()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0)
  {
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    String mensaje = "";
    while (LoRa.available()) {
      mensaje += (char)LoRa.read();
    }
    mensaje.trim();

    Serial.println("Recibido: " + mensaje);
    Serial.print("RSSI: "); Serial.print(rssi); Serial.print(" dBm | ");
    Serial.print("SNR: "); Serial.print(snr, 1); Serial.println(" dB");

    String id = extraerID(mensaje);
    if (id.length() > 0)
    {
      contactosTotales++;

      // Enviar ACK
      String ack = "ACK:" + id;
      LoRa.beginPacket();
      LoRa.print(ack);
      LoRa.endPacket();

      // Mostrar en LCD con 4 líneas
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ID:" + id + " C:" + String(contactosTotales));

      lcd.setCursor(0, 1);
      lcd.print("RSSI:");
      lcd.print(rssi);
      lcd.print("dBm");

      lcd.setCursor(0, 2);
      lcd.print("SNR: ");
      lcd.print(snr, 1); // 1 decimal
      lcd.print("dB");

      lcd.setCursor(0, 3);
      lcd.print("Calidad: ");
      lcd.print(calidadSenal(snr));

      mostrarMensajeTemporal = true;
      tiempoPantallaMensaje = millis();
    }
    else
    {
      // Mensaje no reconocido, pero igual mostramos señal
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Msg invalido");
      lcd.setCursor(0, 1);
      lcd.print("RSSI:");
      lcd.print(rssi);
      lcd.print("dBm");
      lcd.setCursor(0, 2);
      lcd.print("SNR: ");
      lcd.print(snr, 1);
      lcd.print("dB");
      lcd.setCursor(0, 3);
      lcd.print("Calidad: ");
      lcd.print(calidadSenal(snr));

      mostrarMensajeTemporal = true;
      tiempoPantallaMensaje = millis();
    }
  }

  // Volver a pantalla de espera después de 3 segundos
  unsigned long ahora = millis();
  if (mostrarMensajeTemporal && (ahora - tiempoPantallaMensaje >= 3000))
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gateway LoRa Activo");
    lcd.setCursor(0, 1);
    lcd.print("Esperando endpoint...");
    lcd.setCursor(0, 2);
    lcd.print("Contactos: ");
    lcd.print(contactosTotales);
    lcd.setCursor(0, 3);
    lcd.print("433 MHz | SF7");
    mostrarMensajeTemporal = false;
  }

  delay(10);
}