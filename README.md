# Configuración dinámica de WiFi en ESP32

Los contenidos de este repositorio sirven para implementar una **solución IoT con ESP32** que permite configurar dinámicamente la red WiFi **sin necesidad de reprogramar el dispositivo**.

El usuario final puede conectarse al ESP32 en **modo AP** y configurar las credenciales de su red WiFi mediante una interfaz HTTP local.

---

##  Características principales

* Inicio en **modo Access Point (AP)** si no existen credenciales guardadas.
* Portal web simple para configurar SSID y contraseña.
* Almacena credenciales en **NVS (flash)** de forma persistente.
* Reconexión automática a la red WiFi configurada.
* Endpoints HTTP documentados (OpenAPI).
* Código comentado y organizado en el Arduino framework.

---

##  Requisitos de la tarea

- ESP32 inicia en modo AP si no tiene credenciales.
- Interfaz web para ingresar SSID y contraseña.
- Uso de **NVS** (Preferences.h) para memoria no volátil.
- Reconexión automática.
- Documentación técnica (diagramas, código, OpenAPI).
- README estilo wiki con instrucciones.
- Endpoints documentados y colección Postman.
- Diagramas UML de secuencia.
- Botón físico para reset de configuración.

---

##  Arquitectura del sistema

1. **Modo AP**

   * El ESP32 levanta un Access Point (`Chucho_AP`, clave `12345678`).
   * Se expone una API para configurar el WiFi.

2. **Modo STA (cliente)**

   * El ESP32 intenta conectarse con las credenciales guardadas.
   * Si falla → vuelve a modo AP.

3. **Botón físico (GPIO0)**

   * Mantener presionado 5 segundos → resetea configuración.

---

##  Endpoints API

### `POST /wifi-config`

Configura las credenciales de WiFi.

* **Body (x-www-form-urlencoded):**

  * `ssid`: nombre de la red WiFi
  * `password`: contraseña

**Respuesta exitosa (200):**

```json
{
  "status": "ok",
  "message": "WiFi guardado, reiniciando..."
}
```

---

### `POST /reset-config`

Borra credenciales guardadas y reinicia en modo AP.

**Respuesta exitosa (200):**

```json
{
  "status": "ok",
  "message": "Configuración borrada"
}
```

---

##  Estructura del repositorio

```
/ESP32-WiFi-Config
│── main.ino        # Código principal del ESP32
│── swagger.yaml        # Contrato OpenAPI de la API
│── README.md           # Documentación wiki del proyecto
│── diagrams/
│   ├── uml-sequence.png
│   └── arquitectura.png
│── postman_collection.json
```

---

##  Diagramas

* **Diagrama de secuencia (UML):**

  * Usuario → ESP32 (AP) → Envío de credenciales → Guardar en NVS → Reinicio en modo STA → Conexión a router.
* **Diagrama de arquitectura:**

  * ESP32 ↔ Router WiFi ↔ Cliente HTTP (navegador / Postman).

*(Se recomienda generar en draw.io, Lucidchart o PlantUML y subir imágenes a `/diagrams`)*

---

##  Cómo probar el sistema

1. Conectar a la red `Chucho_AP` (clave `12345678`).
2. Abrir navegador en `http://192.168.4.1/`.
3. Hacer `POST /wifi-config` con SSID y password de tu red.
4. El ESP32 se reinicia y conecta a tu WiFi.
5. Usar `POST /reset-config` para borrar configuración.

---

##  Tecnologías usadas

* ESP32 + Arduino Core
* Biblioteca `WiFi.h`
* Biblioteca `WebServer.h`
* Biblioteca `Preferences.h` (NVS storage)
* OpenAPI 3.0 (documentación de API)

---

##  Autores y créditos
* Autores:
  * Nicolas Almonacid
  * Esteban Bernal
  * Santiago Sanchez
* Proyecto desarrollado en el marco de la asignatura de IoT / Ingeniería Informática.
* Documentación generada con ayuda de **inteligencia artificial (ChatGPT)**.

---
