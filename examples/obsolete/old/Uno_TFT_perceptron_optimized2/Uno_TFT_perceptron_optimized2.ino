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

#define DX 64
#define DY -22

void loop() {
  uint16_t color;
  int32_t DXX = DX<<6;
  int32_t DYY = DY<<5;
  int32_t x0 = -2000 +DXX;
  int32_t y0 = 15609 +DYY;
  for (int16_t y=-128; y<1; y++) {
    x0 += -2*y-1;
    y0 += -121;
    int32_t xi = x0;
    int32_t yi = y0;
    for (int16_t x=-120; x<120; x++) {
      color = BLACK;
      xi += 2*x+1;
      yi += y;
      //int8_t xb = xi>>8;
      //int8_t yb = yi>>7;
      //if (xb*xb+yb*yb<750) {
      int16_t xx = xi>>6;
      int16_t yy = yi>>5;
      yy += 120;
      xx += 160;
      if (xx>=0&&yy>=0&&xx<320&&yy<240) {
        SET_X_LOCATION(yy);
        SET_Y_LOCATION(xx);    
        WRITE_BUS(BEGIN_READ_DATA);
        CLOCK_COMMAND;
        DDRD=0;
        PORTC=0b1110110;   // Read clock low (falling edge)
        PORTC=SEND_DATA;   // Read clock high (rising edge)
        PORTC=0b1110110;   // Read clock low (falling edge)
        DELAY1
        uint8_t R=PIND>>3; // Get red channel data ( 3rd bit is mask flag )
        DDRD=0xff;         // Restore PORTD as an output
        if (R<0b11110) R+=2;
        color = (uint16_t)R*0b0000100001000001;
      }
      tft.drawPixel(-x+120, y+160,color);
      tft.drawPixel( x+120,-y+160,color);
    }
}


}



