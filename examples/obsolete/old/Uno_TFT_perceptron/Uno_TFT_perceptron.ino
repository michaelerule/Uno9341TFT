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


for (int i=0; i<256; i++) {
  for (int j=0; j<256; j++) {
    float y = i*4/256.0-2.0;
    float x = j*4/256.0-2.0;
    float xx = x*x-y*y;
    float yy = 2*x*y;
    /*
    float k0 = x + y;
    float k1 = x*k0;
    float k2 = x*(y-x);
    float k3 = y*k0;
    float xx = k1-k3;
    float yy = k1+k2;
    */
    uint16_t color = BLACK;
    int   I = (xx+2.0)/4.0*256.0-64;
    int   J = (yy+2.0)/4.0*256.0+64;
    if (I>=0&&J>=0&&I<256&J<256) {
      // Always pull pixel from the other side of the screen
      if (i<128==I<128) {
        I = 256-I;
        J = 256-J; 
      }
      color = tft.readPixel(J,I);
    }
    color += 0b0001000001000010;
    tft.drawPixel(j,i,color);
  }
}


}



