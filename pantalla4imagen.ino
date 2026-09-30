#include <DIYables_TFT_Round.h>
#include <stdint.h>

// Archivos de imágenes del primer código
#include "img1.h"
#include "img2.h"
#include "img3.h"
#include "img4.h"

// Configuración de pines del segundo código (funcional)
#define PIN_SCLK 12  // SCL / SCK
#define PIN_MOSI 11  // SDA / MOSI
#define PIN_RST   8  // RST
#define PIN_DC   17  // Conectado al pin 'CS' de la pantalla de 6 pines
#define PIN_CS   10  // Chip Select dummy   

DIYables_TFT_GC9A01_Round TFT_display(PIN_RST, PIN_DC, PIN_CS);

// Arreglo con los punteros de las 4 imágenes
const uint16_t* imagenes[] = {
  image1,
  image2,
  image3,
  image4
};

uint16_t SCREEN_WIDTH;
uint16_t SCREEN_HEIGHT;

int imagenActual = -1;

unsigned long ultimoParpadeo = 0;
unsigned long ultimoRandom = 0;

void mostrarImagen(int numero) {
  if (imagenActual == numero) return;
  imagenActual = numero;

  int x = (SCREEN_WIDTH - 240) / 2;
  int y = (SCREEN_HEIGHT - 240) / 2;

  // Se remueve TFT_display.fillScreen(0x0000) para evitar el destello negro

  TFT_display.drawRGBBitmap(
    x,
    y,
    imagenes[numero],
    240,
    240
  );
}

void actualizarPantalla() {
  unsigned long ahora = millis();

  // Animación de parpadeo cada 2 segundos
  if (ahora - ultimoParpadeo >= 2000) {
    mostrarImagen(3);
    delay(200);
    mostrarImagen(0);
    ultimoParpadeo = ahora;
  }

  // Cambio aleatorio de imagen cada 60 segundos
  if (ahora - ultimoRandom >= 60000) {
    int img = random(1, 4);
    mostrarImagen(img);
    delay(3000);
    mostrarImagen(0);
    ultimoRandom = ahora;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println(F("Arduino TFT LCD Display"));

  TFT_display.begin();

  SCREEN_WIDTH = TFT_display.width();
  SCREEN_HEIGHT = TFT_display.height();

  randomSeed(analogRead(0));

  // Muestra la imagen inicial
  mostrarImagen(0);

  Serial.print("Pixeles image1: "); Serial.println(sizeof(image1) / sizeof(image1[0]));
  Serial.print("Pixeles image2: "); Serial.println(sizeof(image2) / sizeof(image2[0]));
  Serial.print("Pixeles image3: "); Serial.println(sizeof(image3) / sizeof(image3[0]));
  Serial.print("Pixeles image4: "); Serial.println(sizeof(image4) / sizeof(image4[0]));
}

void loop() {
  actualizarPantalla();
}

/*
// 'Slugcat Abierto', 240x240px
const uint16_t image1[] PROGMEM = {
	0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 
*/


