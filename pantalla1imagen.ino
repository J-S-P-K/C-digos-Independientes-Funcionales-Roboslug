/*
   Created by DIYables

   This example code is in the public domain

   Product page:
   - https://diyables.io/products/1.28-inch-round-circular-tft-lcd-display-module
   - https://www.amazon.com/dp/B0F9NY6JZ6
*/

#include <DIYables_TFT_Round.h>
#include "monito.h"
#include <stdint.h>

#define WHITE     DIYables_TFT::colorRGB(255, 255, 255)

#define PIN_SCLK 12  // SCL / SCK
#define PIN_MOSI 11  // SDA / MOSI
#define PIN_RST   8  // RST
#define PIN_DC   17  // Conectado al pin 'CS' de la pantalla de 6 pines
#define PIN_CS   10  // Chip Select dummy   
 
DIYables_TFT_GC9A01_Round TFT_display(PIN_RST, PIN_DC, PIN_CS);

int img_width = 240;
int img_height = 240;

uint16_t SCREEN_WIDTH;
uint16_t SCREEN_HEIGHT;

void setup() {
  Serial.begin(9600);

  Serial.println(F("Arduino TFT LCD Display"));

  TFT_display.begin();

  SCREEN_WIDTH = TFT_display.width();
  SCREEN_HEIGHT = TFT_display.height();

  int x = (SCREEN_WIDTH - img_width) / 2;
  int y = (SCREEN_HEIGHT - img_height) / 2;

  TFT_display.fillScreen(WHITE);
  TFT_display.drawRGBBitmap(x, y, image1, img_width, img_height);
}

void loop(void) {
  delay(2000);
  TFT_display.invertDisplay(true);
  
  
}
