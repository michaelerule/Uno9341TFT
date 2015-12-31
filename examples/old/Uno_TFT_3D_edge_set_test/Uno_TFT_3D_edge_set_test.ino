#include <math.h>

#include <Uno_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

// INCLUDE MODEL DATA
#include "bunny.h"

// INCLUDE EDGE SET DATA STRUCTURE
#include "edge_set.h"

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
#define ALPHA 0.75
#define BETA (1.0-ALPHA)
TSPoint q;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

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
  model();
}

// Test the masked HLINE functionality
void loop() {
  test_edge_set();
}

void test_edge_set() {
  uint16_t X0    = 120;
  uint16_t Y0    = 160;
  
  // For mesh rendering we only really need to draw lines.
  // Each line might be drawn by up to two triangles.
  // Therefore, we can just store a list of lines to draw
  // And can get about a 2x speedup in drawing. We only need
  // to store the endpoints of lines as indecies into the
  // vertex buffer. Actually we need a fast memory efficient
  // set implementation to use this, hold off. 
  uint16_t linebuffer[256*2];
  uint16_t *edgeset1 = &linebuffer[0];
  uint16_t *edgeset2 = &linebuffer[256];
  uint8_t   nset1 = 0;
  uint8_t   nset2 = 0;

  // Insert edges
  insert_edge(99,67,edgeset1,&nset1);
  insert_edge(67,99,edgeset1,&nset1);
  insert_edge(100,67,edgeset1,&nset1);
  insert_edge(100,67,edgeset1,&nset1);
  insert_edge(100,99,edgeset1,&nset1);
  insert_edge(99,100,edgeset1,&nset1);
  insert_edge(99,100,edgeset1,&nset1);
  insert_edge(99,67,edgeset1,&nset1);
  insert_edge(67,99,edgeset1,&nset1);
  insert_edge(100,67,edgeset1,&nset1);
  insert_edge(100,67,edgeset1,&nset1);
  insert_edge(100,99,edgeset1,&nset1);
  insert_edge(99,100,edgeset1,&nset1);
  insert_edge(99,100,edgeset1,&nset1);

  tft.setCursor(0,0);
  tft.setTextColor(WHITE,BLACK);
  tft.println(nset1);
  tft.println(edgeset1[0]);

  // Retrieve edges
  for (int i=0; i<nset1; i++) {
    uint8_t *edge = &(((uint8_t *)(edgeset1))[i*2]);
    uint8_t a = edge[0];
    uint8_t b = edge[1];
    tft.println(a);
    tft.println(b);
  }

  // Draw edges from this frame. 
  tft.masking_on = 0;
  uint8_t *edges = (uint8_t *)edgeset1;
  for (uint8_t e=0; e<nset1; e++) {
    uint8_t *edge = &edges[e*2];
    uint8_t a = edge[0];
    uint8_t b = edge[1];
    int8_t x1 = ((int8_t)pgm_read_byte(&vertices[a*3+0]));
    int8_t y1 = ((int8_t)pgm_read_byte(&vertices[a*3+1]));
    int8_t x2 = ((int8_t)pgm_read_byte(&vertices[b*3+0]));
    int8_t y2 = ((int8_t)pgm_read_byte(&vertices[b*3+1]));
    tft.drawLine(x1+X0,y1+Y0,x2+X0,y2+Y0,WHITE);
  }

  delay(100);
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

#define AXLEN 0.5

void model() {

  // Rotated versions of the points are buffered. 
  int8_t buff[NVERTICES*2*2];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTICES*2];
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
  uint16_t linebuffer[MAX_SET_SIZE*2];
  uint16_t *edgeset1 = &linebuffer[0];
  uint16_t *edgeset2 = &linebuffer[MAX_SET_SIZE];
  uint8_t   nset1 = 0;
  uint8_t   nset2 = 0;
  
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

  while (1) 
  {
    // Get user input
    mouse(&dx,&dy);
       
    // Draw next frame
    // Flip the color and masking bit being used
    tft.mask_flag ^= 0b1000;

    // Perform a rotation on the axis. 
    //dx += 0.1;
    //dy += 0.07;
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
    for (i=0; i<NVERTICES; i++) {
      int8_t *q = &vbuff1[i*2];
      const int8_t *v = &vertices[i*3];
      float  x = (float)((int8_t)pgm_read_byte(&v[0]));
      float  y = (float)((int8_t)pgm_read_byte(&v[1]));
      float  z = (float)((int8_t)pgm_read_byte(&v[2]));
      q[0] = (int8_t)((x*abuff1[0]+y*abuff1[3]+z*abuff1[6]));
      q[1] = (int8_t)((x*abuff1[1]+y*abuff1[4]+z*abuff1[7]));
    }

    // Triangles
    for (i=0; i<NTRIANGLES; i++) {
      // get the triangle indecies
      const uint8_t *t = &triangles[i*3];
      uint8_t ip = pgm_read_byte(&t[0]);
      uint8_t iq = pgm_read_byte(&t[1]);
      uint8_t ir = pgm_read_byte(&t[2]);
      // get the points to check the normal to see if we draw this triangle
      int8_t *p = &vbuff1[ip*2];
      int8_t *q = &vbuff1[iq*2];
      int8_t *r = &vbuff1[ir*2];
      // test normal orientation, draw if it is on the front
      if ((int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1])) {
        // add edges to the current edge set
        insert_edge(ip,iq,edgeset1,&nset1);
        insert_edge(iq,ir,edgeset1,&nset1);
        insert_edge(ir,ip,edgeset1,&nset1);
        //insert_edge(99,67,edgeset1,&nset1);
        //insert_edge(67,99,edgeset1,&nset1);
        //insert_edge(100,67,edgeset1,&nset1);
      }
    }

    // Draw edges from this frame. 
    tft.masking_on = 0;
    color = 0b1111011111110111 | tft.mask_flag*0x0101;
    uint8_t *edges = (uint8_t *)edgeset1;
    for (uint8_t e=0; e<nset1; e++) {
      uint8_t *edge = &edges[e*2];
      uint8_t a = edge[0];
      uint8_t b = edge[1];
      int8_t *p = &vbuff1[a*2];
      int8_t *q = &vbuff1[b*2];
      tft.drawLine(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,color);
    }
  
    
    // Erase the edges from the previous frame
    tft.masking_on = 1;
    edges = (uint8_t *)edgeset2;
    for (uint8_t e=0; e<nset2; e++) {
      uint8_t *edge = &edges[e*2];
      uint8_t a = edge[0];
      uint8_t b = edge[1];
      int8_t *p = &vbuff2[a*2];
      int8_t *q = &vbuff2[b*2];
      tft.drawLine(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,BLACK);
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
    
    // Flip edge set buffers and counters
    uint16_t *temp3 = edgeset1;
    edgeset1 = edgeset2;
    edgeset2 = temp3;
    // Clear out edge set 1 for use in the next frame
    nset2 = nset1;
    nset1 = 0;
  }
}


