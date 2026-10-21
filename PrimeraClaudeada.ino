#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include <Bluepad32.h>

// =======================================
// CONFIGURACIONES
// =======================================

// WiFi
#define WIFI_SSID     "Escuela de Innovacion"
#define WIFI_PASSWORD "3scu3la-2020"

// 
#define STREAM_FRAMESIZE FRAMESIZE_QVGA

/*
Opciones de resolución:
*  FRAMESIZE_QVGA → 320x240
*  FRAMESIZE_VGA  → 640x480
*  FRAMESIZE_SVGA → 800x600
*  FRAMESIZE_HD   → 1280x720   (Recomendado para buena definición)
*  FRAMESIZE_UXGA → 1600x1200  (Máxima resolución)
*/

// de 0 a 63, es cuanto se comprime la imagen (mas es menor calidad)
#define JPEG_QUALITY 20 

// Cantidad de buffers. es la cantidad de hilos simulateneos
#define FRAME_BUFFER_COUNT 2

// Frecuencia del reloj de cámara - 10MHz
#define XCLK_FREQ 10000000

// Delay para limitar FPS (0 = transmisión a la máxima velocidad que permita el WiFi)
#define FRAME_DELAY_MS 30

// ---------------------------------------
// PINOUT ESP32-S3 CAM
// ---------------------------------------

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15

#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13


httpd_handle_t server = NULL;

// Motor 1 (rueda izquierda)
#define IN1 47
#define IN2 48

// Motor 2 (rueda derecha)  <-- CAMBIAR por los pines reales de tu segundo motor
#define IN3 41
#define IN4 42

#define PWM_FREQ 1000
#define PWM_RESOLUTION 8   // 0 a 255

// Canales PWM (el core de ESP32 que usa Bluepad32 trabaja con canales)
// CAMBIO DE INTEGRACION: antes 0, 1, 2, 3. Los canales 0 y 1 comparten el timer 0 de LEDC
// con el reloj de la camara (LEDC_CHANNEL_0 / LEDC_TIMER_0 en initCamera) y habrian
// reconfigurado su frecuencia de 10 MHz a 1 kHz. Se usan 4..7 (timers 2 y 3).
#define CANAL_IN1 4
#define CANAL_IN2 5
#define CANAL_IN3 6
#define CANAL_IN4 7

// Variable para guardar la conexion de NUESTRO mando
ControllerPtr miMando = nullptr;

// Para imprimir cada 100 ms sin usar delay()
unsigned long ultimoPrint = 0;

// =======================================
// STREAM HANDLER OPTIMIZADO (Anti-Bloqueos)
// =======================================

esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      // 🟢 EVITA EL BUCLE INFINITO: Si el sensor no está listo por culpa de la red,
      // espera 10ms antes de volver a preguntar en lugar de congelar la CPU.
      delay(10); 
      continue;
    }

    // 🟢 VALIDACIÓN DE ENVÍO: Si el WiFi se desconecta o traba, rompemos el ciclo
    res = httpd_resp_send_chunk(req, "--frame\r\n", 9);
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "Content-Type: image/jpeg\r\n\r\n", 28);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, "\r\n", 2);
    }

    esp_camera_fb_return(fb); // Liberamos el buffer obligatoriamente

    // Si hubo un error enviando datos (red saturada/cliente desconectado), salimos
    if (res != ESP_OK) {
      break;
    }

    if (FRAME_DELAY_MS > 0) {
      delay(FRAME_DELAY_MS);
    }
  }

  return res;
}



// =======================================
// SERVIDOR HTTP
// =======================================

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &uri);
  }
}


// =======================================
// CÁMARA
// =======================================

void initCamera() {
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = XCLK_FREQ;
  config.pixel_format = PIXFORMAT_JPEG;

  // Ajustes de memoria asignados a PSRAM
  config.frame_size   = STREAM_FRAMESIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count     = FRAME_BUFFER_COUNT;

  // CAMERA_GRAB_LATEST, CAMERA_GRAB_WHEN_EMPTY (Habla de cuanto descarta o no los frames)
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; 

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Error inicializando cámara");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, STREAM_FRAMESIZE);
}


// =======================================
// WIFI
// =======================================

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Desactivar sleep para máxima respuesta de red en el streaming
  WiFi.setSleep(false);
}


// 1. Se ejecuta AUTOMATICAMENTE cuando el mando se conecta
void alConectar(ControllerPtr mando) {
    Serial.println("¡Mando conectado con exito!");
    miMando = mando; // Guardamos el mando en nuestra variable
}

// 2. Se ejecuta AUTOMATICAMENTE cuando el mando se desconecta
void alDesconectar(ControllerPtr mando) {
    Serial.println("Mando desconectado.");
    miMando = nullptr; // Vaciamos la variable
}


// =======================================
// SETUP / LOOP
// =======================================

void setup() {
  Serial.begin(115200);
  delay(2000); // Tiempo para abrir el Monitor Serie

  initCamera();
  initWiFi();
  startCameraServer();

  Serial.println("Stream listo:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  // Configuración manual del sensor de imagen
// =======================================
// Configuración de Imagen
// =======================================

sensor_t * s = esp_camera_sensor_get();

    //   Imagen básica
s->set_brightness(s, 0);
//s->set_contrast(s, 0);
//s->set_saturation(s, 0);

    //   Exposición
//s->set_exposure_ctrl(s, 1);
//s->set_ae_level(s, 0);
//s->set_aec_value(s, 300);

    //   Ganancia
//s->set_gain_ctrl(s, 1);
//s->set_agc_gain(s, 0);

    //   Balance de blancos
//s->set_whitebal(s, 1);
//s->set_wb_mode(s, 0);

    //   Correcciones
//s->set_lenc(s, 1);
//s->set_gainceiling(s, (gainceiling_t)0);

    //   Orientación
s->set_hmirror(s, 1);
s->set_vflip(s, 1);

    //   Efectos
//s->set_special_effect(s, 0);


/*
Parámetro            | Mín | Normal | Máx
-----------------------------------------
Brightness           | -2  | 0      | 2
Contrast             | -2  | 0      | 2
Saturation           | -2  | 0      | 2
Sharpness*           | -2  | 0      | 2
Special Effect       |  0  | 0      | 6
White Balance Mode   |  0  | 0      | 4
Exposure Ctrl (AEC)  |  0  | 1      | 1
AEC2                 |  0  | 0      | 1
AE Level             | -2  | 0      | 2
AEC Value            |  0  | ~300   | 1200+
Gain Ctrl (AGC)      |  0  | 1      | 1
Gain Ceiling         |  0  | 0      | 6
BPC                  |  0  | 0      | 1
WPC                  |  0  | 1      | 1
Raw GMA              |  0  | 1      | 1
Lens Correction      |  0  | 0      | 1
H-Mirror             |  0  | 0      | 1
V-Flip               |  0  | 0      | 1
*/

    // Le indicamos a Bluepad32 que funciones usar al conectar/desconectar
    BP32.setup(&alConectar, &alDesconectar);
    BP32.forgetBluetoothKeys();

    // Configuramos los pines de ambos motores como PWM
    ledcSetup(CANAL_IN1, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(IN1, CANAL_IN1);
    ledcSetup(CANAL_IN2, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(IN2, CANAL_IN2);
    ledcSetup(CANAL_IN3, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(IN3, CANAL_IN3);
    ledcSetup(CANAL_IN4, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(IN4, CANAL_IN4);

    // Motores detenidos
    ledcWrite(CANAL_IN1, 0);
    ledcWrite(CANAL_IN2, 0);
    ledcWrite(CANAL_IN3, 0);
    ledcWrite(CANAL_IN4, 0);
} 

void loop() {
    // El servidor HTTP corre en segundo plano de forma asíncrona

    // Actualiza el estado de la conexion Bluetooth
    BP32.update();

    // Si el mando esta conectado y listo
    if (miMando != nullptr && miMando->isConnected()) {
        
        // joystick derecho
        int x = miMando->axisRX();
        int y = miMando->axisRY();

        //botones de dirección
        bool presionUp = (miMando->dpad() & DPAD_UP) != 0;
        bool presionDown = (miMando->dpad() & DPAD_DOWN) != 0;
        bool presionLeft = (miMando->dpad() & DPAD_LEFT) != 0;
        bool presionRight = (miMando->dpad() & DPAD_RIGHT) != 0;

        // ==========================================
        // CONTROL DE MOTORES CON EL JOYSTICK
        // ==========================================

        // Y: arriba (valor negativo) = adelante (positivo)
        int velocidad = map(y, -512, 508, 255, -255);
        // X: derecha = positivo
        int giro = map(x, -512, 508, -255, 255);

        // Mezcla de los dos ejes para las dos ruedas
        int izquierda = velocidad + giro;
        int derecha = velocidad - giro;
        izquierda = constrain(izquierda, -255, 255);
        derecha = constrain(derecha, -255, 255);

        // Motor 1 (izquierda)
        if (izquierda > 0) {
            // SENTIDO 1
            ledcWrite(CANAL_IN1, izquierda);
            ledcWrite(CANAL_IN2, 0);
        } else {
            // SENTIDO 2 (o frenar si es 0)
            ledcWrite(CANAL_IN1, 0);
            ledcWrite(CANAL_IN2, -izquierda);
        }

        // Motor 2 (derecha)
        if (derecha > 0) {
            // SENTIDO 1
            ledcWrite(CANAL_IN3, derecha);
            ledcWrite(CANAL_IN4, 0);
        } else {
            // SENTIDO 2 (o frenar si es 0)
            ledcWrite(CANAL_IN3, 0);
            ledcWrite(CANAL_IN4, -derecha);
        }

        // Imprimimos los valores de forma sencilla (cada 100 ms, sin delay)
        if (millis() - ultimoPrint >= 100) {
            ultimoPrint = millis();

            Serial.print("X: ");
            Serial.print(x);
            Serial.print(" | Y: ");
            Serial.println(y);
            Serial.print(" | Boton UP: ");
            Serial.println(presionUp);
            Serial.print(" | Boton DOWN: ");
            Serial.println(presionDown);
            Serial.print(" | Boton LEFT: ");
            Serial.println(presionLeft);
            Serial.print(" | Boton RIGTH: ");
            Serial.println(presionRight);
        }

    } else {
        // Sin mando conectado: motores detenidos
        ledcWrite(CANAL_IN1, 0);
        ledcWrite(CANAL_IN2, 0);
        ledcWrite(CANAL_IN3, 0);
        ledcWrite(CANAL_IN4, 0);
    }

    // Cede un instante al sistema (evita que salte el watchdog). No es una pausa.
    vTaskDelay(1);
}
