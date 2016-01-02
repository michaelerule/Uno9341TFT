// Replacing the calls to fastVline with calls to fastHline
// because hline is slightly faster. 
// This is the unit test for that
#include "hack.h"
#include "TouchScreen.h"

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 920
#define TS_MAXY 900
#define MINPRESSURE 0
#define MAXPRESSURE 1000
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Uno_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
// If using the shield, all control and data lines are fixed, and
// a simpler declaration can optionally be used:
// Adafruit_TFTLCD tft;

void setup(void) {
  Serial.begin(9600);
  Serial.println(F("TFT LCD test"));
  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  tft.reset();
  uint16_t identifier=0x9341;
  Serial.println(F("Hard-coded ILI9341 LCD driver"));
  tft.begin(identifier);

  Serial.println(F("Benchmark                Time (microseconds)"));
  Serial.print(F("Screen fill              "));
  Serial.println(testFillScreen());
  delay(1000);
  Serial.print(F("Rectangles (filled)      "));
  Serial.println(testFilledRects(YELLOW, MAGENTA));
  delay(1000);
  Serial.print(F("Circles (filled)         "));
  Serial.println(testFilledCircles(10, MAGENTA));
  delay(1000);
  Serial.print(F("Triangles (filled)       "));
  Serial.println(testFilledTriangles());
  delay(1000);
  Serial.print(F("Rounded rects (filled)   "));
  Serial.println(testFilledRoundRects());
  delay(1000);

  Serial.println(F("Done!"));
  
  // Start Cube Demo
  pinMode(13, OUTPUT);
}

void loop(void) {
  cube();
}

unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(BLACK);
  delay(500);
  tft.fillScreen(RED);
  delay(500);
  tft.fillScreen(GREEN);
  delay(500);
  tft.fillScreen(BLUE);
  delay(500);
  tft.fillScreen(BLACK);
  delay(500);
  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2) {
  unsigned long start, t = 0;
  int           n, i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;
  tft.fillScreen(BLACK);
  n = min(tft.width(), tft.height());
  for(i=n; i>0; i-=6) {
    i2    = i / 2;
    start = micros();
    tft.fillRect(cx-i2, cy-i2, i, i, i&1?color1:color2);
    t    += micros() - start;
    // Outlines are not included in timing results
    tft.drawRect(cx-i2, cy-i2, i, i, i&1?color2:color1);
    delay(50);
  }
  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color) {
  unsigned long start;
  int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;

  tft.fillScreen(BLACK);
  start = micros();
  for(x=radius; x<w; x+=r2) {
    for(y=radius; y<h; y+=r2) {
      tft.fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testFilledTriangles() {
  unsigned long start, t = 0;
  int           i, cx = tft.width()  / 2 - 1,
                   cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(cx,cy); i>10; i-=5) {
    start = micros();
    tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      color565(0, i, i));
    t += micros() - start;
    tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
      color565(i, i, 0));
  }

  return t;
}

unsigned long testFilledRoundRects() {
  unsigned long start;
  int           i, i2,
                cx = tft.width()  / 2 - 1,
                cy = tft.height() / 2 - 1;

  tft.fillScreen(BLACK);
  start = micros();
  for(i=min(tft.width(), tft.height()); i>20; i-=6) {
    i2 = i / 2;
    tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, color565(0, i, 0));
  }
  return micros() - start;
}







#include <math.h>
#define NVERTS 8
#define NEDGES 8

#define MAXD 0.4
#define ALPHA 0.5
#define BETA (1.0-ALPHA)

PROGMEM const uint8_t edges[NEDGES*2]={
  0,2,
  0,4,
  1,3,
  1,5,
  2,6,
  4,5,
  3,7,
  6,7};

void cube() {
  // 3 bit color mode
  PORTC=0b1110001;
  PORTB=PORTD=0x39;
  PORTC=0b1110011;
  
  float buff[NVERTS*2*2];
  float *vbuff1 = &buff[0];
  float *vbuff2 = &buff[NVERTS*2];
  uint16_t color = WHITE;
  uint16_t theta_x = 0;
  uint16_t theta_y = 0;
  uint16_t x0    = 120;
  uint16_t y0    = 160;
  tft.fillScreen(BLACK);
  tft.mask_flag = 0;
  TSPoint q;

  // Define 3D axis. We're going to rotate this around
  // Based on user inputs
  
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3];
  
  abuff1[0] = 50;
  abuff1[1] = 0;
  abuff1[2] = 0;
  
  abuff1[3] = 0;
  abuff1[4] = 50;
  abuff1[5] = 0;
  
  abuff1[6] = 0;
  abuff1[7] = 0;
  abuff1[8] = 50;

  int previous_frame_valid = 0;
  float ddx=0,ddy=0,dddx=0,dddy=0,ddddx=0,ddddy=0;
  while (1) 
  {
    PORTC=0b11111111;
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    } else {
      p.x=q.x;
      p.y=q.y;
    }
    float dx = (p.x-q.x)*0.02;
    float dy = (p.y-q.y)*0.02;
    q.x=p.x;
    q.y=p.y;
    if (dx<-MAXD) dx=-MAXD;
    if (dy<-MAXD) dy=-MAXD;
    if (dx> MAXD) dx=MAXD;
    if (dy> MAXD) dy=MAXD;

    // add some damping
    ddx  = ddx *ALPHA+BETA*dx;
    ddy  = ddy *ALPHA+BETA*dy;
    dddx = dddx*ALPHA+BETA*ddx;
    dddy = dddy*ALPHA+BETA*ddy;
    ddddx = ddddx*ALPHA+BETA*dddx;
    ddddy = ddddy*ALPHA+BETA*dddy;
   
    // Draw next frame

    // Flip the color and masking bit being used
    tft.mask_flag ^= 0b1000;

    // Draw the axis.
    // First, we perform a rotation on the axis. 
    float cdx = cos(ddddx);
    float sdx = sin(ddddx);
    float cdy = cos(ddddy);
    float sdy = sin(ddddy);

    uint16_t colors[3] = {RED,GREEN,BLUE};
    for (int j=0; j<3; j++) {
      // Get the current axis orientation
      float *a = &abuff1[j*3];
      float *b = &abuff2[j*3];
      float x = a[0];
      float y = a[1];
      float z = a[2];
      float nx,ny,nz;
      nx =  cdx*x + sdx*z;
      nz =  cdx*z - sdx*x;
      ny =  cdy*y + sdy*nz;
      nz =  cdy*nz - sdy*y;
      b[0] = nx;
      b[1] = ny;
      b[2] = nz;
      // Draw the new line
      tft.masking_on = 0;
      color = colors[j]| tft.mask_flag*0x0101;
      tft.drawLine(x0,y0,nx+x0,ny+y0,color);
      // Erase the old line
      tft.masking_on = 1;
      tft.drawLine(x0,y0,x+x0,y+y0,BLACK);
    }
    
    // Rotate vertices into place
    int i;
    for (i=0; i<NVERTS; i++) {
      float *q = &vbuff1[i*2];
      float  x = (float)(((i<<1)&2)-1);
      float  y = (float)((i&2)-1);
      float  z = (float)(((i>>1)&2)-1);
      float  w = x*abuff1[2]+y*abuff1[5]+z*abuff1[8];
      w = 200/(200-w);
      q[0] = (x*abuff1[0]+y*abuff1[3]+z*abuff1[6])*w+x0;
      q[1] = (x*abuff1[1]+y*abuff1[4]+z*abuff1[7])*w+y0;
    }

    color = WHITE | tft.mask_flag*0x0101;
    // Draw the next frame of the cube
    for (i=0; i<NEDGES; i++) {
      // Draw the new line
      const uint8_t *e = &edges[i*2];
      float *p = &vbuff1[pgm_read_byte(&e[0])*2];
      float *q = &vbuff1[pgm_read_byte(&e[1])*2];
      tft.masking_on = 0;
      tft.drawLine(p[0],p[1],q[0],q[1],color);
      
      // Erase the old line using the masking feature
      p = &vbuff2[pgm_read_byte(&e[0])*2];
      q = &vbuff2[pgm_read_byte(&e[1])*2];
      tft.masking_on = 1;
      tft.drawLine(p[0],p[1],q[0],q[1],BLACK);
    }
    
    // Flip vertex buffers
    float *temp;
    temp = vbuff1;
    vbuff1 = vbuff2;
    vbuff2 = temp;

    // Flip axis buffers
    temp = abuff1;
    abuff1 = abuff2;
    abuff2 = temp;

  }
}

