#include <Bluepad32.h>

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
    Serial.println("\n[EXITO] ¡Bluepad32 ha reconocido el mando!");
    Serial.printf("Modelo detectado: %s\n", ctl->getModelName().c_str());
    
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            myControllers[i] = ctl;
            break;
        }
    }
}

void onDisconnectedController(ControllerPtr ctl) {
    Serial.println("\n[INFO] Mando desconectado.");
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            myControllers[i] = nullptr;
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Tiempo para estabilizar la conexion USB Serial
    Serial.println("=========================================");
    Serial.println("   ESP32-S3 Lista - Esperando mando...  ");
    Serial.println("=========================================");

    BP32.setup(&onConnectedController, &onDisconnectedController);
    
    // Si cambiaste de modo en el mando, descomenta la siguiente linea 
    // una vez para borrar claves guardadas antes:
    BP32.forgetBluetoothKeys(); 
  }

void loop() {
    BP32.update();
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        ControllerPtr myController = myControllers[i];
        if (myController && myController->isConnected()) {
            // Lectura continua para verificar datos
            Serial.printf("X: %4d | Y: %4d | Boton A: %d | Boton B: %d\n", 
                          myController->axisX(), 
                          myController->axisY(), 
                          myController->a(),
                          myController->b());
        }
    }
    delay(100);
}
