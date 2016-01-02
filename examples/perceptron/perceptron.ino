#include <Arduino_3D.h> 

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

Arduino_3D tft;

#define MINPRESSURE 0
#define MAXPRESSURE 1000
#define NDAMP 4
float dampx[NDAMP];
float dampy[NDAMP];
TSPoint q;

void setup() {
  tft.begin();
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  q.x = 102;
  q.y = 210;
}

#define ALPHA 0.9
#define BETA (1.0-ALPHA)
void getTouch(TSPoint *d) {
    CONTROLPORT=0b11111111;
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
  int32_t x0 = -2000;
  int32_t y0 = 15609;
  for (int16_t y=-128; y<1; y++) {
    x0 += -2*y-1;
    y0 += -121;
    
    getTouch(&p);
    tft.setCursor(0,0);
    tft.setTextColor(WHITE,BLACK);
    tft.println(p.x);
    tft.println(p.y);
    int32_t DX = p.y-160;
    int32_t DY = p.x-120;
    
    int32_t xi = x0+(DX<<6);
    int32_t yi = y0+(DY<<5);
    for (int16_t x=-120; x<120; x++) {
      xi += 2*x+1;
      yi += y;
      int16_t xx = xi>>6;
      int16_t yy = yi>>5;
      yy += 120;
      xx += 160;
      if (xx>=32&&yy>=0&&xx<288&&yy<240) {
        SET_XY_LOCATION(yy,xx);
        COMMAND(BEGIN_READ_DATA);
        setReadDir();
        READY_READ;   // Read clock low (falling edge)
        SEND_DATA;    // Read clock high (rising edge)
        DELAY1
        READY_READ;   // Read clock low (falling edge)
        DELAY1
        uint8_t R=QUICK_READ>>3; // Get red channel data ( 3rd bit is mask flag )
        setWriteDir();         // Restore PORTD as an output
        if (R<0b11111) R++;
        color = R*0b0000100001000001;
      } else color=BLACK;
      tft.fastPixel(-x+120, y+160,color);
      tft.fastPixel( x+120,-y+160,color);
    }
  }
}



