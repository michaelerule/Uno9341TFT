#include <math.h>

#include <Uno_TFTLCD.h> // Hardware-specific library
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

Uno_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  triangle_overlap_test();
}

/*  Because adjacent triangles overlap,
 *  we sometimes see flickering when drawing polyhedral surfaces
 *  As trianges of different colors may repeatedly overdraw the
 *  same screen areas on every other frame. 
 *  To avoid this, we are defining edge conditions such that 
 *  adjacent triangles do not overlap. The rightmost and bottom-
 *  most edges of the triangle will be exclusive, and will be
 *  skipped. This code is a unit test to verify that triangle
 *  overlap is properly addressed. It repeatedly draws 6 triangles
 *  in alternatig color. If overlap is correctly addressed, there
 *  should be no flicker. 
 */
#define PI 3.141592653589793f
void triangle_overlap_test() {     
  const int x0 = 120;
  const int y0 = 160;
  const int r  = 99;
  int polarity = 1;
  while (1)  {
    for (int i=0; i<6; i++) {
      float theta1 = (float)i*(2*PI)/6.f;
      float theta2 = (float)(i+1)*(2*PI)/6.f;
      int x1 = x0 + r*cos(theta1);
      int y1 = y0 + r*sin(theta1);
      int x2 = x0 + r*cos(theta2);
      int y2 = y0 + r*sin(theta2);
      uint16_t color = ((i&1)^polarity) ? RED:GREEN;
      tft.fillTriangle(x0,y0,x1,y1,x2,y2,color);
    }
    //polarity = 1-polarity;
    delay(50);
  }
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


#define NVERTS     110

PROGMEM const int8_t vertices[NVERTS*3]={
  22,    0, -119,  -21,    0, -119,   48,   42, -103,   26,   78,
 -89,  -25,   78,  -89,   90,   26,  -77,  120,   22,    0,   90,
 -25,  -77,  104,  -47,  -41,   26,  -77,  -89,   48,  -41, -103,
-103,  -47,  -41,  -89,  -25,  -77,  -41, -103,  -47,  -25,  -77,
 -89,  -47,  -41, -103,  -89,   26,  -77,  -77,   90,  -25,  -47,
  42, -103,    0,  120,   22,  -41,  104,   48,  -41,  104,  -47,
 -77,   90,   26,    0,  120,  -21,   42,  104,  -47,   78,   90,
  26,  -77,  -89,  -25,   42, -103,  -47,   78,  -89,  -25,    0,
-119,  -21,    0, -119,   22,   78,  -89,   26,   22,    0,  120,
 -47,   42,  104,   26,   78,   90,   48,   42,  104,   42,  104,
  48,   90,   26,   78,  104,   48,   42,   90,  -25,   78,  120,
 -21,    0,  104,  -47,   42,   48,  -41,  104,   42, -103,   48,
 -25,  -77,   90,  -47,  -41,  104,   26,  -77,   90,  -21,    0,
 120,  -89,  -25,   78,  -41, -103,   48,  -77,  -89,   26,  -89,
  26,   78, -119,   22,    0, -103,  -47,   42, -119,  -21,    0,
-103,   48,   42,  -25,   78,   90,  104,   48,  -41,   78,   90,
 -25,   90,   26,  -77,   48,   42, -103,   90,  -25,  -77,   48,
 -41, -103,  -25,   78,  -89,  -41,  104,  -47,    0,  120,  -21,
  26,   78,  -89,   42,  104,  -47,  -47,  -41, -103,  -89,  -25,
 -77,  -89,   26,  -77,  -41, -103,  -47,  -25,  -77,  -89,   26,
 -77,  -89,   42, -103,  -47,    0, -119,  -21,   90,  -25,   78,
  90,   26,   78,   48,   42,  104,   48,  -41,  104,  -25,  -77,
  90,  -41, -103,   48,    0, -119,   22,   42, -103,   48,   26,
 -77,   90,  -47,   42,  104,  -89,   26,   78,  -21,    0,  120,
 -89,  -25,   78,  -47,  -41,  104,   42,  104,   48,   26,   78,
  90,    0,  120,   22,  -41,  104,   48,  104,  -47,   42,   78,
 -89,   26,   78,  -89,  -25,  104,  -47,  -41,   78,   90,  -25,
  78,   90,   26,  104,   48,  -41,  104,   48,   42,  -77,   90,
  26, -103,   48,  -41, -103,   48,   42, -119,   22,    0, -103,
 -47,   42, -119,  -21,    0,  -77,  -89,   26,  -77,  -89,  -25};
        
#define NTRIANGLES 54

PROGMEM const uint8_t triangles[NTRIANGLES*4]={

9,    8,   6, 0b11010101,  
 9,   6,  41, 0b11010101,  
41,   6,   7, 0b11010101,  
 6,  58,   7, 0b11010101,  

35,  36,  38, 0b00101001, 
35,  38,  37, 0b00101001,  
37,  38,  26, 0b00101001, 
38,  39,  26, 0b00101001, 

42,  40,  43, 0b11001111,
42,  43,  32, 0b11001111, 
32,  43,  44, 0b11001111,  
43,  47,  44, 0b11001111,   

34,  48,  33, 0b11100000, 
34,  33,  57, 0b11100000,  
57,  33,  35, 0b11100000, 
33,  36,  35, 0b11100000, 

45,  46,  49, 0b10000111,
45,  49,  50, 0b10000111,  
50,  49,  51, 0b10000111,  
49,  54,  51, 0b10000111, 

14,  30,  31, 0b01001100, 
14,  31,  27, 0b01001100,  
27,  31,  51, 0b01001100,
31,  50,  51, 0b01001100, 

1,   11,  10, 0b11111111,  
1,  10,   2,  0b11111111,   
2,   10,  16, 0b11111111,  
10,  15,  16, 0b11111111,  

13,  12,  55, 0b01010101,  
13,  55,  17, 0b01010101,  
17,  55, 104, 0b01010101,  
55,  53, 104, 0b01010101, 

20,  24,  22, 0b10101010, 
20,  22,  21, 0b10101010,  
21,  22,  23, 0b10101010, 
22,  18,  23, 0b10101010, 

64,  65,  66, 0b11001100,  
64,  66,  67, 0b11001100,  
66,  68,  67, 0b11001100,  

103,  18, 104, 0b10011111, 
103, 104, 105, 0b10011111, 
104, 106, 105, 0b10011111, 

41,  95,  96, 0b11001001,  
41,  96,  98, 0b11001001,  
96,  97,  98, 0b11001001, 

101, 99, 100, 0b11110001, 
101, 100,   7, 0b11110001, 
100, 102,   7, 0b11110001, 

88,  86,  87, 0b00001110,  
88,  87,  90, 0b00001110,  
87,  89,  90, 0b00001110,  

72,  73,  74, 0b11011111,  
72,  74,  76, 0b11011111,
74,  75,  76, 0b11011111,  
};


void model() {
  int8_t buff[NVERTS*2*2];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTS*2];
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
  abuff1[0] = .2;
  abuff1[1] = 0;
  abuff1[2] = 0;
  abuff1[3] = 0;
  abuff1[4] = .2;
  abuff1[5] = 0;
  abuff1[6] = 0;
  abuff1[7] = 0;
  abuff1[8] = .2;

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
      int8_t *q = &vbuff1[i*2];
      const int8_t *v = &vertices[i*3];
      float  x = (float)((int8_t)pgm_read_byte(&v[0]));
      float  y = (float)((int8_t)pgm_read_byte(&v[1]));
      float  z = (float)((int8_t)pgm_read_byte(&v[2]));
      float  w = x*abuff1[2]+y*abuff1[5]+z*abuff1[8];
      w = 80/(80-w);
      q[0] = (int8_t)((x*abuff1[0]+y*abuff1[3]+z*abuff1[6])*w);
      q[1] = (int8_t)((x*abuff1[1]+y*abuff1[4]+z*abuff1[7])*w);
    }

    // Draw the next frame of the cube
    
    // Triangles
    tft.masking_off();
    for (i=0; i<NTRIANGLES; i++) {
      // Draw the new triangle
      const uint8_t *t = &triangles[i*4];
      int8_t *p = &vbuff1[pgm_read_byte(&t[0])*2-2];
      int8_t *q = &vbuff1[pgm_read_byte(&t[1])*2-2];
      int8_t *r = &vbuff1[pgm_read_byte(&t[2])*2-2];
      if ((int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1])) {
        color = (pgm_read_byte(&t[3])&0b11110111 | tft.mask_flag)*0x0101;
        tft.fillTriangle(p[0]+x0,p[1]+y0,q[0]+x0,q[1]+y0,r[0]+x0,r[1]+y0,color);
      }
    }
    tft.masking_on(); 
    for (i=0; i<NTRIANGLES; i++) {
      // Erase the old line using the masking feature
      const uint8_t *t = &triangles[i*4];
      int8_t *p = &vbuff2[pgm_read_byte(&t[0])*2-2];
      int8_t *q = &vbuff2[pgm_read_byte(&t[1])*2-2];
      int8_t *r = &vbuff2[pgm_read_byte(&t[2])*2-2];
      if ((int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1])) {
        tft.fillTriangle(p[0]+px,p[1]+py,q[0]+px,q[1]+py,r[0]+px,r[1]+py,BLACK);
      }
    }
    
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


