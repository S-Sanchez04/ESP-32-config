// ==== Librerías necesarias ====
#include <WiFi.h>          // Librería para manejo de conexión WiFi en ESP32
#include <WebServer.h>     // Librería para crear un servidor web HTTP
#include <Preferences.h>   // Librería para guardar datos persistentes en memoria flash (NVS)

// ==== Definición de constantes ====
#define RESET_PIN 0        // Pin físico (GPIO0) usado como botón de reseteo

// ==== Objetos globales ====
Preferences preferences;   // Objeto para acceder a la memoria NVS
WebServer server(80);      // Servidor web escuchando en el puerto 80 (HTTP)

// ==== Variables globales ====
String mode;               // Guarda el modo actual: "no_config" o "config"
String savedSSID;          // Guarda el SSID cargado desde memoria
String savedPassword;      // Guarda la contraseña cargada desde memoria

// ==== Función para guardar credenciales WiFi en NVS ====
void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);         // Abre el espacio de memoria "wifi" en modo escritura
  preferences.putString("ssid", ssid);      // Guarda el SSID como string
  preferences.putString("password", password); // Guarda la contraseña como string
  preferences.putString("mode", "config");  // Guarda el modo actual como "config"
  preferences.end();                        // Cierra el acceso a NVS
  Serial.println("Credenciales guardadas en NVS"); // Mensaje de confirmación en consola
}

// ==== Función para borrar la configuración WiFi ====
void resetWiFiConfig() {
  preferences.begin("wifi", false);   // Abre el espacio "wifi" en modo escritura
  preferences.clear();                // Borra todos los datos guardados
  preferences.end();                  // Cierra la memoria
  Serial.println("Configuración borrada. Reiniciando en modo no_config..."); // Mensaje informativo
  delay(1000);                        // Espera 1 segundo antes de reiniciar
  ESP.restart();                      // Reinicia el ESP32
}

// ==== Función para cargar credenciales desde NVS ====
bool loadWiFiConfig() {
  preferences.begin("wifi", true);                  // Abre el espacio "wifi" en modo solo lectura
  savedSSID = preferences.getString("ssid", "");    // Lee el SSID (o vacío si no existe)
  savedPassword = preferences.getString("password", ""); // Lee la contraseña (o vacío)
  mode = preferences.getString("mode", "no_config");     // Lee el modo ("config" o "no_config")
  preferences.end();                                // Cierra la memoria
  // Retorna true si tanto SSID como contraseña tienen contenido
  return (savedSSID.length() > 0 && savedPassword.length() > 0);
}

// ==== Endpoint HTTP: POST /wifi-config ====
// Permite recibir las credenciales desde un cliente web
void handleWiFiConfig() {
  if (server.method() == HTTP_POST) {            // Verifica que el método sea POST
    if (server.hasArg("ssid") && server.hasArg("password")) {  // Verifica que se enviaron los argumentos necesarios
      String ssid = server.arg("ssid");          // Obtiene el valor del SSID desde la petición
      String password = server.arg("password");  // Obtiene el valor de la contraseña

      saveWiFiCredentials(ssid.c_str(), password.c_str()); // Guarda las credenciales en memoria

      // Envía una respuesta JSON de éxito
      server.send(200, "application/json", "{\"status\":\"ok\", \"message\":\"WiFi guardado, reiniciando...\"}");
      delay(1000);                               // Espera un segundo
      ESP.restart();                             // Reinicia el ESP32 para aplicar cambios
    } else {
      // Si faltan argumentos, responde con error 400 (Bad Request)
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Faltan ssid o password\"}");
    }
  } else {
    // Si el método no es POST, responde con error 405 (Método no permitido)
    server.send(405, "application/json", "{\"status\":\"error\", \"message\":\"Método no permitido\"}");
  }
}

// ==== Endpoint HTTP: POST /reset-config ====
// Permite borrar la configuración desde una petición web
void handleResetConfig() {
  // Responde con mensaje JSON de confirmación
  server.send(200, "application/json", "{\"status\":\"ok\", \"message\":\"Configuración borrada\"}");
  // Llama a la función para borrar configuración y reiniciar
  resetWiFiConfig();
}

// ==== Función para iniciar el ESP32 en modo Access Point (AP) ====
void startAPMode() {
  WiFi.mode(WIFI_AP);                        // Configura el ESP32 en modo punto de acceso
  WiFi.softAP("Chucho_AP", "12345678");      // Crea la red WiFi con nombre "Chucho_AP" y contraseña "12345678"
  IPAddress IP = WiFi.softAPIP();            // Obtiene la IP del AP
  Serial.print("Access Point iniciado. IP: "); // Muestra en consola
  Serial.println(IP);                        // Imprime la IP del AP

  server.on("/wifi-config", handleWiFiConfig);   // Asocia el endpoint /wifi-config con su manejador
  server.on("/reset-config", handleResetConfig); // Asocia el endpoint /reset-config
  server.begin();                             // Inicia el servidor web
  Serial.println("Servidor HTTP en modo AP listo"); // Mensaje de confirmación
}

// ==== Función para iniciar el ESP32 en modo cliente (STA) ====
void startSTAMode() {
  WiFi.mode(WIFI_STA);                       // Configura el ESP32 como cliente WiFi
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str()); // Intenta conectar con las credenciales guardadas

  Serial.print("🔌 Conectando a WiFi: ");     // Muestra SSID en consola
  Serial.println(savedSSID);

  unsigned long startAttemptTime = millis();  // Guarda el tiempo actual para control de timeout
  // Intenta conectarse durante 15 segundos
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
    Serial.print(".");                        // Imprime puntos mientras intenta conectar
    delay(500);                               // Espera medio segundo
  }

  // Si logra conectarse
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Conectado exitosamente!"); // Mensaje de éxito
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());           // Muestra IP obtenida

    server.on("/reset-config", handleResetConfig); // Habilita el endpoint de reset
    server.begin();                           // Inicia servidor HTTP
    Serial.println("Servidor HTTP en modo STA listo"); // Confirma servidor activo
  } else {
    // Si no logra conectar después del tiempo límite
    Serial.println("\n No se pudo conectar. Volviendo a modo no_config...");
    resetWiFiConfig();                        // Borra la configuración y reinicia
  }
}

// ==== Función para revisar el botón físico de reseteo ====
void checkResetButton() {
  if (digitalRead(RESET_PIN) == LOW) {        // Verifica si el botón está presionado (nivel bajo)
    unsigned long pressedTime = millis();     // Registra el momento en que se presionó
    // Mientras el botón siga presionado
    while (digitalRead(RESET_PIN) == LOW) {
      // Si se mantiene presionado más de 5 segundos
      if (millis() - pressedTime > 5000) {
        Serial.println("Botón presionado largo. Reseteando configuración..."); // Mensaje en consola
        resetWiFiConfig();                  // Borra la configuración y reinicia
      }
    }
  }
}

// ==== Setup inicial del ESP32 ====
void setup() {
  Serial.begin(115200);                       // Inicia comunicación serial a 115200 baudios
  pinMode(RESET_PIN, INPUT_PULLUP);           // Configura el pin del botón como entrada con pull-up interno

  // Si hay credenciales guardadas y el modo es "config"
  if (loadWiFiConfig() && mode == "config") {
    startSTAMode();                           // Inicia en modo cliente WiFi
  } else {
    startAPMode();                            // De lo contrario, inicia en modo AP para configuración
  }
}

// ==== Bucle principal ====
void loop() {
  server.handleClient();                      // Atiende peticiones HTTP entrantes
  checkResetButton();                         // Revisa constantemente si se presiona el botón físico
}
