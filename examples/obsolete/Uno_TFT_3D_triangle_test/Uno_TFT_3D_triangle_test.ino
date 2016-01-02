#include <Uno_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!secret
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
  //cube();
}


// Test the masked HLINE functionality
void loop() {
  mask_debug();
}


void mask_debug() {
  tft.masking_on = 0;
  tft.fillScreen(BLACK);
  tft.mask_flag = 0;

  // Pixels matching the secret mask flag ARE NOT drawn
  // Secret mask flag is lowest bit of 5-bit Red channel
  // Draw two lines, one with and one without that bit set.
  // Later, we'll test masking by trying to erase both of them.
  uint16_t color1 = 0b1111011111111111;
  uint16_t color2 = 0b1111111111111111;
  uint16_t color3 = 0b0000011111111111;
  uint16_t color4 = 0b0000111111111111;
  
  /*
  tft.drawFastHLine(10, 60,70,color1);
  tft.drawFastHLine(10,120,70,color2);
  tft.drawFastHLine(10,180,70,color3);
  tft.drawFastHLine(10,240,70,color4);
  tft.drawFastHLine(160, 60,70,color1);
  tft.drawFastHLine(160,120,70,color2);
  tft.drawFastHLine(160,180,70,color3);
  tft.drawFastHLine(160,240,70,color4);
  */
  
  tft.drawLine(10, 60,10+70, 60,color1);
  tft.drawLine(10,120,10+70,120,color2);
  tft.drawLine(10,180,10+70,180,color3);
  tft.drawLine(10,240,10+70,240,color4);
  tft.drawLine(160, 60,160+70, 60,color1);
  tft.drawLine(160,120,160+70,120,color2);
  tft.drawLine(160,180,160+70,180,color3);
  tft.drawLine(160,240,160+70,240,color4);
  
  // This is not working, so let's do a test read. 
  uint32_t pixel1 = tft.readPixel( 59,319); // pixel from line 1
  uint32_t pixel2 = tft.readPixel(10, 60); // pixel from line 1
  uint32_t pixel3 = tft.readPixel(10,120); // pixel from line 2
  uint32_t pixel4 = tft.readPixel(10,180); // pixel from line 2
  uint32_t pixel5 = tft.readPixel(10,240); // pixel from line 2

  // Print out the pixel values
  tft.setCursor(0, 0);
  tft.setTextColor(WHITE);  tft.setTextSize(1);
  for (int i=31; i>=0; i--) {
    tft.print(pixel1>>i&0b1);
    if (i%8==0) tft.print(' ');
  }
  tft.println("");
  for (int i=31; i>=0; i--) {
    tft.print(pixel2>>i&0b1);
    if (i%8==0) tft.print(' ');
  }
  tft.println("");
  for (int i=31; i>=0; i--) {
    tft.print(pixel3>>i&0b1);
    if (i%8==0) tft.print(' ');
  }
  tft.println("");
  for (int i=31; i>=0; i--) {
    tft.print(pixel4>>i&0b1);
    if (i%8==0) tft.print(' ');
  }
  tft.println("");
  for (int i=31; i>=0; i--) {
    tft.print(pixel5>>i&0b1);
    if (i%8==0) tft.print(' ');
  }
  tft.println("");
  

  // Pixes are sent out in 565 RGB format
  // But for some reason are read back in 888 RGB format.
  // The internal representation of colors is 6 bit. 
  // They are padded to 8 bit when returned. 
  // The lowest order bit in a 5-bit red value should be
  // the second bit of the 6-bit.
  tft.mask_flag  = 0b1000;
  tft.masking_on = 1;
  delay(2000);

  // Erase both lines. Only one of them should be erased in theory.
  
  tft.drawFastHLine(30, 60,200,GREEN);
  tft.drawFastHLine(30,120,200,GREEN);
  tft.drawFastHLine(10,180,200,GREEN);
  tft.drawFastHLine(10,240,200,GREEN);
  tft.drawFastHLine(30, 62,200,RED);
  tft.drawFastHLine(30,122,200,RED);
  tft.drawFastHLine(10,182,200,RED);
  tft.drawFastHLine(10,242,200,RED);
  
  /*
  tft.drawLine(30, 60,30+200, 60,GREEN);
  tft.drawLine(30,120,30+200,120,GREEN);
  tft.drawLine(10,180,10+200,180,GREEN);
  tft.drawLine(10,240,10+200,240,GREEN);
  tft.drawLine(30, 62,30+200, 62,RED);
  tft.drawLine(30,122,30+200,122,RED);
  tft.drawLine(10,182,10+200,182,RED);
  tft.drawLine(10,242,10+200,242,RED);
  */
  
  delay(5000);
}


#include <math.h>
#define NVERTS 8
#define NEDGES 8
#define NTRIANGLES 10

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
};

void cube() {

  // 3 bit color mode
  /*
  PORTC=0b1110001;
  PORTB=PORTD=0x39;
  PORTC=0b1110011;
 */
  
  float buff[NVERTS*2*2];
  float *vbuff1 = &buff[0];
  float *vbuff2 = &buff[NVERTS*2];
  uint16_t color = WHITE;
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

  float ddx=0,ddy=0,dddx=0,dddy=0,ddddx=0,ddddy=0;
  while (1) 
  {
    // Do touch screen stuff
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
    if (dx>MAXD)  dx=MAXD;
    if (dy>MAXD)  dy=MAXD;
    // add some damping to the cursor movement
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
      nz   = cdx*z  - sdx*x;      
      b[0] = nx = cdx*x  + sdx*z;
      b[1] = ny = cdy*y  + sdy*nz;
      b[2] = nz = cdy*nz - sdy*y;
      /*
      // Draw the new line
      tft.masking_on = 0;
      color = colors[j]|tft.mask_flag*0x0101;
      tft.drawLine(x0,y0,nx+x0,ny+y0,color);
      // Erase the old line
      tft.masking_on = 1;
      tft.drawLine(x0,y0,x+x0,y+y0,BLACK);
      */
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

    
    // Draw the next frame of the cube
    
    // Triangles
    tft.masking_on = 0;
    for (i=0; i<NTRIANGLES; i++) {
      // Draw the new triangle
      const uint8_t *t = &triangles[i*4];
      float *p = &vbuff1[pgm_read_byte(&t[0])*2];
      float *q = &vbuff1[pgm_read_byte(&t[1])*2];
      float *r = &vbuff1[pgm_read_byte(&t[2])*2];
      float z_normal = (q[0]-p[0])*(r[1]-p[1])-(r[0]-p[0])*(q[1]-p[1]);
      if (z_normal<0) {
        color = (pgm_read_byte(&t[3])&0b11110111 | tft.mask_flag)*0x0101;
        /*
        uint32_t c = pgm_read_byte(&t[3]);
        float light = (p[0]-q[0])*0.004;
        c *= light;
        c = c&0b11110100;
        color = (c | tft.mask_flag)*0x0101;
        */
        tft.fillTriangle(p[0],p[1],q[0],q[1],r[0],r[1],color);
      }
    }
    tft.masking_on = 1;    
    for (i=0; i<NTRIANGLES; i++) {
      // Erase the old line using the masking feature
      const uint8_t *t = &triangles[i*4];
      float *p = &vbuff2[pgm_read_byte(&t[0])*2];
      float *q = &vbuff2[pgm_read_byte(&t[1])*2];
      float *r = &vbuff2[pgm_read_byte(&t[2])*2];
      float z_normal = (q[0]-p[0])*(r[1]-p[1])-(r[0]-p[0])*(q[1]-p[1]);
      if (z_normal<0) {
        tft.fillTriangle(p[0],p[1],q[0],q[1],r[0],r[1],BLACK);
      }
    }
    /*
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
    */
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


