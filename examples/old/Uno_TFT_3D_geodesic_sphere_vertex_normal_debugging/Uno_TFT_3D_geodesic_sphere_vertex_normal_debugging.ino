#include <math.h>

#include <Uno_3D.h> // Hardware-specific library
#include <TouchScreen.h>

// INCLUDE MODEL DATA
//#include "sphere.h"
//#include "bunny.h"
#include "face.h"

// CONFIGURE TOUCH SCREEN STUFF
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
#define MAXD 0.8
#define ALPHA 0.5
#define BETA (1.0-ALPHA)
TSPoint q;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// CONFIGURE LCD LINES. NOT SURE IF NEEDED
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
  //triangle_overlap_test();
  model();
}

// Test the masked HLINE functionality
void loop() {
}


float ddx=0,ddy=0,dddx=0,dddy=0,ddddx=0,ddddy=0;
void mouse(float *DX, float *DY) {
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
    *DX = ddddx;
    *DY = ddddy;
}

#define AXLEN 0.7

void model() {

  // Rotated versions of the points are buffered. 
  int8_t buff[NVERTICES*2*3];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTICES*3];
  uint16_t color = WHITE;
  uint16_t x0    = 120;
  uint16_t y0    = 160;
  tft.fillScreen(BLACK);
  tft.mask_flag = 0;

  // For mesh rendering we only really need to draw lines.
  // Each line might be drawn by up to two triangles.
  // Therefore, we can just store a list of lines to draw
  // And can get about a 2x speedup in drawing. We only need
  // to store the endpoints of lines as indecies into the
  // vertex buffer. Actually we need a fast memory efficient
  // set implementation to use this, hold off. 
  // uint8_t linebuffer[
  

  // Define 3D axis. We're going to rotate this around
  // Based on user inputs
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3];  
  abuff1[0] = AXLEN;
  abuff1[1] = 0;
  abuff1[2] = 0;
  abuff1[3] = 0;
  abuff1[4] = AXLEN;
  abuff1[5] = 0;
  abuff1[6] = 0;
  abuff1[7] = 0;
  abuff1[8] = AXLEN;

  // mouse tracking  
  const int X0 = 120;
  const int Y0 = 160;
  float dx, dy;

  // Initialize permutation. We keep this across frames
  // Because sorting a mostly ordered set is faster. 
  uint8_t permutation[NTRIANGLES];
  for (int i=0; i<NTRIANGLES; i++) permutation[i]=i;

  while (1) 
  {
    // Get user input
    mouse(&dx,&dy);
       
    // Draw next frame
    // Flip the color and masking bit being used
    tft.mask_flag ^= 0b1000;

    // Draw the axis.
    // First, we perform a rotation on the axis. 
    dx += 0.01;
    dy += 0.07;
    float cdx = cos(dx);
    float sdx = sin(dx);
    float cdy = cos(dy);
    float sdy = sin(dy);
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
    // Also compute normals at this point
    uint16_t normal_colors[NVERTICES];
    int i;
    for (i=0; i<NVERTICES; i++) {
      int8_t *q = &vbuff1[i*3];
      const int8_t *v = &vertices[i*3];
      float  x = (float)((int8_t)pgm_read_byte(&v[0]));
      float  y = (float)((int8_t)pgm_read_byte(&v[1]));
      float  z = (float)((int8_t)pgm_read_byte(&v[2]));
      q[0] = (int8_t)((x*abuff1[0]+y*abuff1[3]+z*abuff1[6]));
      q[1] = (int8_t)((x*abuff1[1]+y*abuff1[4]+z*abuff1[7]));
      q[2] = (int8_t)((x*abuff1[2]+y*abuff1[5]+z*abuff1[8]));

      // Use the precomputed normals
      int8_t nx, ny, nz, lx, ly, lz;
      const int8_t *normal;
      normal = &normals[i*3];
      nx = (int8_t)pgm_read_byte(&normal[0]);
      ny = (int8_t)pgm_read_byte(&normal[1]);
      nz = (int8_t)pgm_read_byte(&normal[2]);
      lz = nx*abuff1[2]+ny*abuff1[5]+nz*abuff1[8];
      lz /= 4;
      if (lz<0) lz=0;
      normal_colors[i] = 0b0000100001000001*lz;
    }

    // Compute the approximate Z locations of all faces to draw in 
    // the correct order. 
    uint8_t depths[NTRIANGLES];
    for (int j=0; j<NTRIANGLES; j++) {
      int i = permutation[j];
      // Get the vertex indecies for the triangle
      const uint8_t *t = &triangles[i*3];
      uint8_t pi = pgm_read_byte(&t[0]);
      uint8_t qi = pgm_read_byte(&t[1]);
      uint8_t ri = pgm_read_byte(&t[2]);
      // get the rotated vertex Z coordinates for the triangle
      int8_t z = (vbuff1[pi*3+2] + vbuff1[qi*3+2] + vbuff1[ri*3+2]) / 3;
      depths[j] = z;
    }
    
    // Bubble sort the triangles by depth keeping track of the permutation
    uint8_t sorted = 0;
    while (!sorted) {
      sorted = 1;
      for (i=1;i<NTRIANGLES;i++) {
        int8_t d1 = depths[i-1];
        int8_t d2 = depths[i];
        if (d2>d1) {
          depths[i-1] = d2;
          depths[i]   = d1;
          uint8_t temp = permutation[i];
          permutation[i] = permutation[i-1];
          permutation[i-1] = temp;
          sorted = 0;
        }
      }
    }
    
    // Triangles
    tft.masking_off();
    tft.overdraw_on();
    
    for (int j=0; j<NTRIANGLES; j++) {
      i = permutation[j];
      // Draw the new triangle
      const uint8_t *t = &triangles[i*3];
      // Get the vertex indecies for the triangle
      uint8_t pi = pgm_read_byte(&t[0]);
      uint8_t qi = pgm_read_byte(&t[1]);
      uint8_t ri = pgm_read_byte(&t[2]);
      // get the rotated vertex X and Y coordinates for the triangle
      int8_t *p = &vbuff1[pi*3];
      int8_t *q = &vbuff1[qi*3];
      int8_t *r = &vbuff1[ri*3];
      if ((int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1])) {
        uint16_t color1 = normal_colors[pi];
        uint16_t color2 = normal_colors[qi];
        uint16_t color3 = normal_colors[ri];
        tft.shadeTriangle(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,r[0]+X0,r[1]+Y0,color1,color2,color3);
      }
    }
    
    tft.masking_on();    
    for (i=0; i<NTRIANGLES; i++) {
      // Erase the old line using the masking feature
      const uint8_t *t = &triangles[i*3];
      int8_t *p = &vbuff2[((int)pgm_read_byte(&t[0]))*3];
      int8_t *q = &vbuff2[((int)pgm_read_byte(&t[1]))*3];
      int8_t *r = &vbuff2[((int)pgm_read_byte(&t[2]))*3];
      if ((int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1])) {
        tft.fillTriangle(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,r[0]+X0,r[1]+Y0,BLACK);
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
    

  }
}


