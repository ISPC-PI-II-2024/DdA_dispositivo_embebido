// web_portal.cpp
#include "web_portal.h"
#include "conexiones.h"
#include <WebServer.h>
#include <Preferences.h>

// Usar las variables globales de conexiones.h
extern Preferences preferences;
extern String ssid;
extern String password;
extern bool wifiConfigured;

WebServer server(80);

const char* HTML_FORM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Configurar WiFi</title>
  <style>
    body { font-family: Arial; margin: 20px; background: #f0f0f0; }
    .container { max-width: 400px; margin: auto; background: white; padding: 20px; border-radius: 8px; }
    h1 { color: #333; }
    input { width: 100%; padding: 10px; margin: 8px 0; box-sizing: border-box; }
    button { width: 100%; padding: 12px; background: #4CAF50; color: white; border: none; cursor: pointer; }
    button:hover { background: #45a049; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Gateway MQTT</h1>
    <h2>Configurar WiFi</h2>
    <form action="/save" method="POST">
      <label>SSID:</label>
      <input type="text" name="ssid" required>
      <label>Contraseña:</label>
      <input type="password" name="password" required>
      <button type="submit">Guardar y Conectar</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_FORM);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String newSsid = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // Guardar en Preferences
    preferences.putString("ssid", newSsid);
    preferences.putString("password", newPassword);
    
    // Actualizar variables globales
    ssid = newSsid;
    password = newPassword;
    wifiConfigured = true;
    
    server.send(200, "text/html", 
      "<html><body><h1>Guardado!</h1><p>Reiniciando...</p></body></html>");
    
    delay(2000);
    ESP.restart();
  } else {
    server.send(400, "text/html", 
      "<html><body><h1>Error</h1><p>Datos incompletos</p></body></html>");
  }
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.println("Servidor web iniciado en 192.168.4.1");
}

void handleWebRequests() {
  server.handleClient();
}