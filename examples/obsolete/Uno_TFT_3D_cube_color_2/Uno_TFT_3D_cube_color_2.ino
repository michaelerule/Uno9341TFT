#include <math.h>

#include <Uno_3D.h> // Hardware-specific library
#include <TouchScreen.h>

#include <TouchScreen.h>
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!secret
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 920
#define TS_MAXY 900
#define MINPRESSURE 2
#define MAXPRESSURE 1000
TSPoint q;
// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

Uno_3D tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  model();
}
// Test the masked HLINE functionality
void loop() {
}


#define RADIUS 50
#define NDAMP 5
float dampx[NDAMP];
float dampy[NDAMP];
#define ALPHA 0.3
#define BETA (1.0-ALPHA)
void getTouch(TSPoint *d) {
    PORTC=0b11111111;
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      float x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      float y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
      if (x<RADIUS) x=RADIUS;
      if (y<RADIUS) y=RADIUS;
      if (x>=240-RADIUS) x=240-RADIUS-1;
      if (y>=320-RADIUS) y=320-RADIUS-1;
      q.x = x;
      q.y = y;
    }
    dampx[0] = q.x;
    dampy[0] = q.y;
    for (int i=1; i<NDAMP; i++) {
      float dx = (dampx[i]-dampx[i-1])*(ALPHA-1);
      float dy = (dampy[i]-dampy[i-1])*(ALPHA-1);
      float v = 0.001+sqrt(dx*dx+dy*dy);
      float w = v;
      if (v>5) v=5;
      dampx[i] += (dx*v)/w;
      dampy[i] += (dy*v)/w;
    }
    d->x = dampx[NDAMP-1];
    d->y = dampy[NDAMP-1];   
}


#include <math.h>
#define NVERTS 8
#define NTRIANGLES 12

#define MAXD 0.4
#define ALPHA 0.45
#define BETA (1.0-ALPHA)

PROGMEM const int8_t vertices[NVERTS*3]={
  1,1,1,
  1,1,-1,
  1,-1,1,
  1,-1,-1,
  -1,1,1,
  -1,1,-1,
  -1,-1,1,
  -1,-1,-1};

PROGMEM const uint8_t triangles[NTRIANGLES*4]={
  0,2,6,CYAN,
  0,6,4,CYAN,
  0,4,5,RED,
  0,5,1,RED,
  0,3,2,BLUE,
  0,1,3,BLUE,
  1,7,3,YELLOW,
  1,5,7,YELLOW,
  2,3,7,GREEN,
  2,7,6,GREEN,
  4,6,7,MAGENTA,
  4,7,5,MAGENTA,
};


void model() {
  int8_t buff[NVERTS*2*3];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTS*3];
  uint16_t color = WHITE;
  uint16_t x0    = 120;
  uint16_t y0    = 160;
  tft.fillScreen(BLACK);
  tft.mask_flag = 0;

  // Define 3D axis. We're going to rotate this around
  // Based on user inputs
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3];  
  abuff1[0] = 20;
  abuff1[1] = 0;
  abuff1[2] = 0;
  abuff1[3] = 0;
  abuff1[4] = 20;
  abuff1[5] = 0;
  abuff1[6] = 0;
  abuff1[7] = 0;
  abuff1[8] = 20;

  // mouse tracking  
  TSPoint p;
  float px=q.x,py=q.y;
  float dx=0,dy=0;
  q.x = 120;
  q.y = 160;
  p.x = 120;
  p.y = 160;

  while (1) 
  {
    // Do touch screen stuff
    getTouch(&p);
    x0 = p.x;
    y0 = p.y;
    dx = (x0-px)*0.02+0.07;
    dy = (y0-py)*0.02+0.05;
       
    // Draw next frame
    // Flip the color and masking bit being used
    tft.mask_flag ^= 0b1000;

    // Draw the axis.
    // First, we perform a rotation on the axis. 
    float cdx = cos(dx);
    float sdx = sin(dx);
    float cdy = cos(dy);
    float sdy = sin(dy);
    uint16_t colors[3] = {WHITE,WHITE,WHITE};
    for (int j=0; j<3; j++) {
      // Get the current axis orientation
      float *a = &abuff1[j*3];
      float *b = &abuff2[j*3];
      float x = a[0];
      float y = a[1];
      float z = a[2];
      float nx,ny,nz;
      nz   = cdx*z  - sdx*x;      
      b[0] = nx = cdx*x  + sdx*z;
      b[1] = ny = cdy*y  + sdy*nz;
      b[2] = nz = cdy*nz - sdy*y;
    }
    
    // Rotate vertices into place
    int i;
    for (i=0; i<NVERTS; i++) {
      int8_t *q = &vbuff1[i*3];
      const int8_t *v = &vertices[i*3];
      float  x = (float)((int8_t)pgm_read_byte(&v[0]));
      float  y = (float)((int8_t)pgm_read_byte(&v[1]));
      float  z = (float)((int8_t)pgm_read_byte(&v[2]));
      float  w = x*abuff1[2]+y*abuff1[5]+z*abuff1[8];
      w = 180/(180-w);
      q[0] = (int8_t)((x*abuff1[0]+y*abuff1[3]+z*abuff1[6])*w);
      q[1] = (int8_t)((x*abuff1[1]+y*abuff1[4]+z*abuff1[7])*w);
    }

    // Draw the next frame of the cube
    
    // Triangles
    tft.masking_off();
    for (i=0; i<NTRIANGLES; i++) {
      // Draw the new triangle
      const uint8_t *t = &triangles[i*4];
      int8_t *p = &vbuff1[pgm_read_byte(&t[0])*3];
      int8_t *q = &vbuff1[pgm_read_byte(&t[1])*3];
      int8_t *r = &vbuff1[pgm_read_byte(&t[2])*3];
      if ((int)(r[0]-p[0])*(q[1]-p[1])>(int)(q[0]-p[0])*(r[1]-p[1])) {
        color = (pgm_read_byte(&t[3])&0b11110111 | tft.mask_flag)*0x0101;
        tft.fastFillTriangle(p[0]+x0,p[1]+y0,q[0]+x0,q[1]+y0,r[0]+x0,r[1]+y0,color);
      }
    }
    tft.setLocation(px,py);
    tft.eraseBoundingBox(vbuff2,(uint16_t)NVERTS);
    
    // Flip vertex buffers
    int8_t *temp;
    temp = vbuff1;
    vbuff1 = vbuff2;
    vbuff2 = temp;

    // Flip axis buffers
    float *temp2 = abuff1;
    abuff1 = abuff2;
    abuff2 = temp2;
    
    // update previous location data;
    px = x0;
    py = y0;

  }
}


