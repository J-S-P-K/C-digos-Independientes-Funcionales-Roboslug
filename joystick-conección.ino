#include <Bluepad32.h>

// Variable para guardar la conexion de NUESTRO mando
ControllerPtr miMando = nullptr;

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

        // Imprimimos los valores de forma sencilla
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

    delay(100); // Pausa de 0.1 segundos entre lecturas
}
