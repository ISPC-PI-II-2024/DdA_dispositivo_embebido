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
  <title>Configuración WiFi ESP32</title>
  <style>
    /* Estilos para el cuerpo y fondo */
    body { 
        font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
        margin: 0; 
        background: linear-gradient(135deg, #1c3d52 0%, #3a6878 100%); /* Degradado Azul Oscuro */
        color: #333;
        display: flex;
        justify-content: center;
        align-items: center;
        min-height: 100vh;
    }
    /* Contenedor principal de la tarjeta */
    .container { 
        max-width: 360px; 
        width: 90%; /* Ajuste responsivo */
        background: #ffffff; 
        padding: 30px; 
        border-radius: 12px;
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2); /* Sombra suave */
    }
    /* Títulos */
    h1 { 
        color: #1c3d52; 
        text-align: center; 
        margin-bottom: 5px;
        font-size: 1.8em;
    }
    h2 {
        color: #3a6878;
        text-align: center;
        margin-top: 5px;
        border-bottom: 2px solid #e0e0e0;
        padding-bottom: 10px;
        font-size: 1.2em;
    }
    /* Etiquetas de los campos */
    label { 
        display: block; 
        margin-top: 15px; 
        margin-bottom: 5px;
        font-weight: bold;
        color: #555;
    }
    /* Campos de entrada de texto */
    input[type="text"], input[type="password"] { 
        width: 100%; 
        padding: 12px; 
        margin: 0; 
        border: 1px solid #ccc;
        border-radius: 6px; 
        box-sizing: border-box;
        transition: border-color 0.3s;
    }
    input:focus {
        border-color: #4CAF50;
        outline: none;
        box-shadow: 0 0 5px rgba(76, 175, 80, 0.5);
    }
    /* Botón */
    button { 
        width: 100%; 
        padding: 12px; 
        margin-top: 25px;
        background: #4CAF50; /* Verde principal */
        color: white; 
        border: none; 
        border-radius: 6px;
        cursor: pointer;
        font-size: 1.1em;
        transition: background 0.3s, transform 0.1s;
    }
    button:hover { 
        background: #45a049; 
        box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
    }
    button:active {
        transform: scale(0.99);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Gateway MQTT</h1>
    <h2>Configurar WiFi</h2>
    <form action="/save" method="POST">
      <label>SSID:</label>
      <input type="text" name="ssid" required placeholder="Nombre de la red">
      <label>Contraseña (Opcional):</label>
      <input type="password" name="password" placeholder="Deja vacío para red abierta">
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