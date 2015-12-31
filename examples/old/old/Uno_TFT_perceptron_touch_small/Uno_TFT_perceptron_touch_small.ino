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

#define MINPRESSURE 0
#define MAXPRESSURE 1000
#define NDAMP 2
#define ALPHA 0.5
#define BETA (1.0-ALPHA)
float dampx[NDAMP];
float dampy[NDAMP];
TSPoint q;


void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  q.x = 158;
  q.y = 271;
}

void getTouch(TSPoint *d) {
    PORTC=0b11111111;
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      q.x = d->x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      q.y = d->y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    }
    dampx[0] = q.x;
    dampy[0] = q.y;
    for (int i=1; i<NDAMP; i++) {
      dampx[i] = dampx[i]*ALPHA + BETA*dampx[i-1]; 
      dampy[i] = dampy[i]*ALPHA + BETA*dampy[i-1]; 
    }
    d->x = dampx[NDAMP-1];
    d->y = dampy[NDAMP-1];   
}
  
void loop() {
  TSPoint p;
  uint16_t color;
 
  getTouch(&p);
  tft.setCursor(0,0);
  tft.setTextColor(WHITE,BLACK);
  tft.println(p.x);
  tft.println(p.y);
  int32_t DX = p.y-160<<4;
  int32_t DY = p.x-120<<3;

  int32_t x0 = -1000+DX;
  int32_t y0 = 7804+DY;

  for (int16_t y=-64; y<1; y++) {
    x0 -= 4*y+1;
    y0 -= 121;    
    int32_t xi = x0;
    int32_t yi = y0>>1;
    for (int16_t x=-60; x<60; x++) {
      xi += 4*x+1;
      yi += y;
      int16_t xx = xi>>6;
      int16_t yy = yi>>4;
      if (xx>=-64&&yy>=-64&&xx<64&&yy<64) {
        yy += 120;
        xx += 160;
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
        if (R<0b11111) R++;
        color = R*0b0000100001000001;
      } else color=BLACK;
      tft.drawPixel(-x+120, y+160,color);
      tft.drawPixel( x+120,-y+160,color);
    }
  }
}



