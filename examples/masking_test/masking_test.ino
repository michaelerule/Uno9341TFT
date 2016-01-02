#include <math.h>
#include <Arduino_3D.h> 

Arduino_3D tft;

void setup() {
  tft.begin();
  tft.background_color = BLACK;
  tft.foreground_color = WHITE;
  tft.fastFillScreen(BLACK);
  tft.overdraw_off();
  tft.flip_mask();
}

#define DELAY 500

void loop() { 
  tft.fastFillTriangle( 0,0,   120,0,  0,160,  YELLOW);
  tft.fastFillTriangle( 240,0, 120,0,  240,160, ORANGE);
  tft.fastFillTriangle( 240,320, 120,320, 240,160, GREEN);
  tft.flip_mask();
  tft.fastFillTriangle( 2,320,  120,320,   0,160, LIME);
  delay(DELAY);
  tft.eraseRegion(0,0,240,320);
  delay(DELAY);
  tft.flip_mask();
}

