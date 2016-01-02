/* Fast color interpolation will ne necessary for phong shading
   (alternative is to just use a single int8 scalar and a LUT)
*/

#include <math.h>
#include <Uno_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

// CONFIGURE LCD LINES. NOT SURE IF NEEDED
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Uno_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  color_interpolation_test();
}

void color_interpolation_test() {

  tft.shadeTriangle(120, 0, 120, 320, 240, 160, BLACK, WHITE, MAGENTA);
  tft.shadeTriangle(120, 0, 120, 320, 0, 160, BLACK, WHITE, GREEN);
  
  uint16_t color1 = CYAN;
  uint16_t color2 = MAGENTA;
  uint16_t color3 = YELLOW;
  uint16_t color4 = BLACK;

  for (int y=0; y<64; y++) {
  for (int x=0; x<60; x++) {
    int alpha = (float)x*256.0/60.0;
    int beta  = (float)y*256.0/64.0;
    uint16_t colora = tft.interpolate(color1,color3,alpha);
    uint16_t colorb = tft.interpolate(color2,color4,alpha);
    uint16_t color  = tft.interpolate(colora,colorb,beta);
    tft.drawPixel(x,y,color);
  }
  }
  // Simulating shading a triangle
  for (int y=80; y<320; y++) {
  uint16_t color1 = tft.interpolate(MAGENTA,YELLOW,y-80);
  uint16_t color2 = tft.interpolate(CYAN,YELLOW,y-80);
  tft.interpolateFastHLine(120-(y-80)/2,y,y-80,color1,color2);
  }
  tft.shadeTriangle(30,300,210,300,120,130,YELLOW,MAGENTA,CYAN);

  triangle_test();
}

#define PI 3.141592653589793f

void triangle_test() {
  const int x0 = 120;
  const int y0 = 160;
  const int r  = 30;
  int polarity = 1;
  const uint16_t colors[6] = {RED, MAGENTA, BLUE, CYAN, GREEN, YELLOW};
  float delta = 0.15;
  while (1)  {
    for (int i = 0; i < 6; i++) {
      float theta1 = (float)i * (2 * PI) / 6.f + delta;
      float theta2 = (float)(i + 1) * (2 * PI) / 6.f + delta;
      int x1 = x0 + r * cos(theta1);
      int y1 = y0 + r * sin(theta1);
      int x2 = x0 + r * cos(theta2);
      int y2 = y0 + r * sin(theta2);
      uint16_t color = ((i & 1)^polarity) ? RED : GREEN;
      tft.shadeTriangle(x0, y0, x1, y1, x2, y2, BLACK, colors[i], colors[(i + 1) % 6]);
    }
    //polarity = 1-polarity;
    delay(50);
    delta += 0.1;
  }
}

// Test the masked HLINE functionality
void loop() {
}



