#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#define RESET_PIN 0   // Pin físico para reset (GPIO0 en la mayoría de ESP32)

Preferences preferences;   // Para guardar config en NVS (flash)
WebServer server(80);

String mode;               // "no_config" o "config"
String savedSSID;
String savedPassword;

// ==== Guardar credenciales ====
void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("mode", "config");
  preferences.end();
  Serial.println("Credenciales guardadas en NVS");
}

// ==== Borrar configuración ====
void resetWiFiConfig() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  Serial.println("Configuración borrada. Reiniciando en modo no_config...");
  delay(1000);
  ESP.restart();
}

// ==== Cargar configuración ====
bool loadWiFiConfig() {
  preferences.begin("wifi", true);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  mode = preferences.getString("mode", "no_config");
  preferences.end();
  return (savedSSID.length() > 0 && savedPassword.length() > 0);
}

// ==== Endpoint: POST /wifi-config ====
void handleWiFiConfig() {
  if (server.method() == HTTP_POST) {
    if (server.hasArg("ssid") && server.hasArg("password")) {
      String ssid = server.arg("ssid");
      String password = server.arg("password");

      saveWiFiCredentials(ssid.c_str(), password.c_str());

      server.send(200, "application/json", "{\"status\":\"ok\", \"message\":\"WiFi guardado, reiniciando...\"}");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Faltan ssid o password\"}");
    }
  } else {
    server.send(405, "application/json", "{\"status\":\"error\", \"message\":\"Método no permitido\"}");
  }
}

// ==== Endpoint: POST /reset-config ====
void handleResetConfig() {
  server.send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Configuración borrada\"}");
  resetWiFiConfig();
}

// ==== Modo AP ====
void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Chucho_AP", "12345678");  // SSID y contraseña del AP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point iniciado. IP: ");
  Serial.println(IP);

  server.on("/wifi-config", handleWiFiConfig);
  server.on("/reset-config", handleResetConfig);
  server.begin();
  Serial.println("Servidor HTTP en modo AP listo");
}

// ==== Modo STA (conectar a WiFi real) ====
void startSTAMode() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

  Serial.print("🔌 Conectando a WiFi: ");
  Serial.println(savedSSID);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado exitosamente!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/reset-config", handleResetConfig);
    server.begin();
    Serial.println("Servidor HTTP en modo STA listo");
  } else {
    Serial.println("\n No se pudo conectar. Volviendo a modo no_config...");
    resetWiFiConfig(); 
  }
}


// ==== Revisión del botón físico ====
void checkResetButton() {
  if (digitalRead(RESET_PIN) == LOW) { // botón presionado
    unsigned long pressedTime = millis();
    while (digitalRead(RESET_PIN) == LOW) {
      if (millis() - pressedTime > 5000) { // 5 segundos
        Serial.println("Botón presionado largo. Reseteando configuración...");
        resetWiFiConfig();
      }
    }
  }
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  pinMode(RESET_PIN, INPUT_PULLUP); // Botón a GND

  if (loadWiFiConfig() && mode == "config") {
    startSTAMode();
  } else {
    startAPMode();
  }
}

// ==== Loop ====
void loop() {
  server.handleClient();
  checkResetButton();
}
