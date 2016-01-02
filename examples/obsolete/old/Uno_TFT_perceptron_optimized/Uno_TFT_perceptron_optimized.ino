#include <Uno_GFX.h>    // Core graphics library
#include <Uno_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 920
#define TS_MAXY 900

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Uno_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

//#define MINPRESSURE 10
#define MINPRESSURE 0
#define MAXPRESSURE 1000

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
}

void loop() {
uint16_t color;
#define DX 64
#define DY 0
for (int16_t y=-128; y<128; y++) {
  for (int16_t x=-120; x<120; x++) {
    color = BLACK;
    int16_t k0 = y+x;
    int16_t k1 = x*k0;
    int32_t xx = ((k1-y*k0)>>6)+DX;
    int32_t yy = ((k1+x*(y-x))>>6)+DY;
    int16_t  Y = xx+128;
    int16_t  X = yy+120;
    if (xx*xx+yy*yy<16384) {
    //if (!(X&0xffffff00)) {
      uint8_t R = (tft.readHigh(X,Y)>>3);
      if (R<0b11111) R++;
      color = (uint16_t)R*0b0000100001000001;
    }
    tft.drawPixel(x+120,y+128,color);
  }
}


}



