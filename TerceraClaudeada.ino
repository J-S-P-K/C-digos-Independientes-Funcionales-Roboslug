#include <Bluepad32.h>

// Motor 1 (rueda izquierda)
#define IN1 47
#define IN2 48

// Motor 2 (rueda derecha)  <-- CAMBIAR por los pines reales de tu segundo motor
#define IN3 41
#define IN4 42

#define PWM_FREQ 1000
#define PWM_RESOLUTION 8   // 0 a 255

// Canales PWM (el core de ESP32 que usa Bluepad32 trabaja con canales)
#define CANAL_IN1 0
#define CANAL_IN2 1
#define CANAL_IN3 2
#define CANAL_IN4 3

// Variable para guardar la conexion de NUESTRO mando
ControllerPtr miMando = nullptr;

// Para imprimir cada 100 ms sin usar delay()
unsigned long ultimoPrint = 0;

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

void setup() {
    Serial.begin(115200);
    delay(2000); // Tiempo para abrir el Monitor Serie

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
