#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// =======================================
// CONFIGURACIONES DE CÁMARA
// =======================================

// Resolución del streaming
#define STREAM_FRAMESIZE FRAMESIZE_QVGA

// Calidad JPEG (10 a 63: menor número = mayor calidad de imagen)
#define JPEG_QUALITY 12 

// Cantidad de buffers en PSRAM
#define FRAME_BUFFER_COUNT 2

// Frecuencia del reloj de cámara (20MHz recomendado para ESP32-S3)
#define XCLK_FREQ 10000000

// Delay para limitar FPS (0 = velocidad máxima de transmisión)
#define FRAME_DELAY_MS 10

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

// =======================================
// LECTURA DE PUERTO SERIE
// =======================================

String readSerialInput() {
  while (!Serial.available()) {
    delay(10);
  }
  String input = Serial.readStringUntil('\n');
  input.trim(); // Elimina espacios en blanco y saltos de línea (\r)
  return input;
}

// =======================================
// ESCANEO Y CONEXIÓN WIFI INTERACTIVA
// =======================================

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("\n--- ESCANEANDO REDES WIFI DISPONIBLES ---");
  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("No se encontraron redes WiFi disponibles.");
  } else {
    Serial.printf("%d redes encontradas:\n", n);
    for (int i = 0; i < n; ++i) {
      Serial.printf("%2d: %-25s | RSSI: %d dBm | %s\n", 
                    i + 1, 
                    WiFi.SSID(i).c_str(), 
                    WiFi.RSSI(i), 
                    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Abierta" : "Protegida");
      delay(10);
    }
  }

  Serial.println("\nEscribe el nombre de la red (SSID) y presiona Enter:");
  String ssid = readSerialInput();
  Serial.printf("SSID seleccionado: %s\n", ssid.c_str());

  Serial.println("Escribe la contraseña y presiona Enter:");
  String password = readSerialInput();

  Serial.printf("Conectando a %s ", ssid.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 30) { // Timeout de 15 segundos
      Serial.println("\nNo se pudo conectar a la red. Reiniciando el ESP32...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi conectado exitosamente!");
  Serial.print("Dirección IP asignada: ");
  Serial.println(WiFi.localIP());

  WiFi.setSleep(false);
}

// =======================================
// STREAM HANDLER OPTIMIZADO
// =======================================

esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  if (res != ESP_OK) return res;

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      delay(10); 
      continue;
    }

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

    esp_camera_fb_return(fb);

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

  config.frame_size   = STREAM_FRAMESIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count     = FRAME_BUFFER_COUNT;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_LATEST; 

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error inicializando cámara: 0x%x\n", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_framesize(s, STREAM_FRAMESIZE);
  }
}

// =======================================
// SETUP / LOOP
// =======================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Inicializar la cámara PRIMERO (antes de encender el WiFi)
  initCamera();

  // 2. Escanear redes y conectar interactivamente
  initWiFi();

  // 3. Iniciar el servidor HTTP
  startCameraServer();

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_brightness(s, 0);   
    s->set_hmirror(s, 1);      
    s->set_vflip(s, 1);        
  }

  Serial.println("\nStream listo. Abre en el navegador:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
}


void loop() {
  // Servidor procesando en segundo plano
}
