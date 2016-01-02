#define TURBO_MODE

#include <Uno_GFX.h>    // Core graphics library
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
  
  mask_test();
  mask_debug();
  //color_test();
  //delay(500);
  //cube();
}

void loop() {
}


void mask_test() {
  /* Test the masking feature. We need to be able to do the following:
   *  Draw a line
   *  Draw the next frame for the line in-place
   *  Erase the previous line
   *  Repeat
   *  
   *  Notes: masked drawing skips point if
   *  mask_flag matches the current pixel data
   *  so the "new frame" pixels should match mask_flag
   *  
   *  This test currently passes.
   */

  uint16_t color1 = 0b1111011111111111;
  uint16_t color2 = 0b1111111111111111;
  
  // clear screen
  tft.masking_off();
  tft.fastFillScreen(BLACK);

  // Draw a line with mask_flag off
  tft.fastLine(0,0,240,320,color1);
  delay(500);

  // Draw the "next line" with mask_flag set
  tft.fastLine(0,1,240,321,color2);
  delay(500);

  // Erase the old line by overdrawing in black
  // The old frame has mask_flag off
  // The new frame has mask_flag set
  // Pixels are skipped (not erased) if 
  // mask_flag equals the pixel data
  // so mask_flag set should match that
  // of the "new" frame, i.e. 
  // mask_flag set should be set in this case
  tft.mask_flag = 0b1000;
  tft.masking_on();
  tft.drawLine(0,0,240,320,BLACK);
  delay(500);

  // We should be able to repeat that for the next frame
  
  // Draw a line with mask_flag off
  tft.masking_off();
  tft.fastLine(0,2,240,322,color1);
  delay(500);

  tft.mask_flag = 0b0000;
  tft.masking_on();
  tft.drawLine(0,0,240,320,BLACK);
  delay(500);
  
}

void mask_debug() {
  tft.masking_off();
  tft.fastFillScreen(BLACK);
  tft.mask_flag = 0;

  // Pixels matching the secret mask flag ARE NOT drawn
  // Secret mask flag is lowest bit of 5-bit Red channel
  // Draw two lines, one with and one without that bit set.
  // Later, we'll test masking by trying to erase both of them.
  uint16_t color1 = 0b1111011111111111;
  uint16_t color2 = 0b1111111111111111;
  uint16_t color3 = 0b0000011111111111;
  uint16_t color4 = 0b0000111111111111;
  tft.fastLine(  0,0, 60,320,color1);
  tft.fastLine( 60,0,120,320,color2);
  tft.fastLine(120,0,180,320,color3);
  tft.fastLine(180,0,240,320,color4);

  
  // This is not working, so let's do a test read. 
  uint32_t pixel1 = tft.readPixel( 59,319); // pixel from line 1
  uint32_t pixel2 = tft.readPixel(0,0); // pixel from line 1
  uint32_t pixel3 = tft.readPixel( 60,0); // pixel from line 2
  uint32_t pixel4 = tft.readPixel(120,0); // pixel from line 2
  uint32_t pixel5 = tft.readPixel(180,0); // pixel from line 2

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
  tft.masking_on();
  delay(2000);

  // Erase both lines. Only one of them should be erased in theory.
  //tft.drawLine(  0,0,120,320,BLACK);
  //tft.drawLine(120,0,240,320,BLACK);

  tft.drawLine(120,0,180,320,BLACK);
  tft.drawLine(180,0,240,320,BLACK);
  tft.drawLine(  0,0, 60,320,BLACK);
  tft.drawLine( 60,0,120,320,BLACK);
  
  delay(2000);
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
      tft.masking_off();
      color = colors[j]| tft.mask_flag*0x0101;
      tft.drawLine(x0,y0,nx+x0,ny+y0,color);
      // Erase the old line
      tft.masking_on();
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
      tft.masking_off();
      tft.drawLine(p[0],p[1],q[0],q[1],color);
      
      // Erase the old line using the masking feature
      p = &vbuff2[pgm_read_byte(&e[0])*2];
      q = &vbuff2[pgm_read_byte(&e[1])*2];
      tft.masking_on();
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

void color_test() {
  tft.fillScreen(BLACK);
  uint8_t i=0;
  for (int y=0; y<320; y+=15) {
    for (int x=0; x<240; x+=15) {
      uint16_t color = (i<<8)|i;
      tft.fillRect(x,y,15,15,color);
      if (i==255) break;
      i++;
    }
    if (i==255) break;
  }
  tft.fillRect(00,300,20,20,BLACK);
  tft.fillRect(20,300,20,20,WHITE);
  tft.fillRect(40,300,20,20,RED);
  tft.fillRect(60,300,20,20,GREEN);
  tft.fillRect(80,300,20,20,BLUE);
  tft.fillRect(100,300,20,20,YELLOW);
  tft.fillRect(120,300,20,20,MAGENTA);
  tft.fillRect(140,300,20,20,CYAN);
}



