#include <SPI.h>
#include <stdint.h>

#define LCD_RST 18
#define LCD_CS  17
#define LCD_RS  16
#define LCD_WR  15
#define LCD_RD  14

#define LCD_D0 8
#define LCD_D1 9
#define LCD_D2 2
#define LCD_D3 3
#define LCD_D4 4
#define LCD_D5 5
#define LCD_D6 6
#define LCD_D7 7

#define SD_DD  10
#define SD_DI  11
#define SD_DO  12
#define SD_SCK 13

#include "LCD_lib.h"

#define CANVAS_SIZE 160

void setup() {
  initialize_LCD();
  clear_background();
  set_background(0b11101101);

  // restrict drawing area
  write_command(0x2A);
  write_data((240-CANVAS_SIZE)/2);
  write_data(240-(240-CANVAS_SIZE)/2-1);
  
  write_command(0x2B);
  write_data((320-CANVAS_SIZE)/2);
  write_data(320-(320-CANVAS_SIZE)/2-1);
}

void clear_background() {
  write_command(START_FRAME);
  for (int32_t j=0; j<76800; j++) {
    PORTC=0b1110101;
    PORTB=PORTD=j;
    PORTC=0b1110111;
    PORTC=0b1110101;
    PORTB=PORTD=j;
    PORTC=0b1110111;
    PORTC=0b1110101;
    PORTB=PORTD=j;
    PORTC=0b1110111;
  }
}

void set_background(uint8_t color)
{
  uint8_t R = color   &0b11100000;
  uint8_t G = color<<3&0b11100000;
  uint8_t B = color<<6&0b11000000;
  R |= R>>3;
  G |= G>>3;
  B |= (B>>2)|(B>>4);
  write_command(START_FRAME); 
  for (int32_t j=0; j<76800; j++) {
    PORTC=0b1110101;
    PORTB=PORTD=B;
    PORTC=0b1110111;
    PORTC=0b1110101;
    PORTB=PORTD=G;
    PORTC=0b1110111;
    PORTC=0b1110101;
    PORTB=PORTD=R;
    PORTC=0b1110111;
  }
}  


uint32_t shift = 0;

void stripe()
{
  write_command(START_FRAME);
  PORTC=0b1110101;
  uint16_t color = 0b11100111;
  uint8_t R = color   &0b11100000;
  uint8_t G = color<<3&0b11100000;
  uint8_t B = color<<6&0b11000000;
  int k;
  k=0; 
  for (uint16_t i=0; i<CANVAS_SIZE; i++)
  {
    for (uint16_t j=0; j<CANVAS_SIZE; j++)
    {
      color = i+j+shift;
      PORTC=0b1110101;
      PORTB=PORTD=color>>1;
      PORTC=0b1110111;
      PORTC=0b1110101;
      PORTB=PORTD=color<<2;
      PORTC=0b1110111;
      PORTC=0b1110101;
      PORTB=PORTD=color<<5;
      PORTC=0b1110111;
    }
  }
  shift-=2;
  PORTC=0b1111011;
}  


float fshift = 0.f;

void quasi()
{
  //start_frame();

  int8_t ct[5];
  int8_t st[5];
  for (int k=0; k<5; k++) {
    float theta = k*6.283185307179586f/5;
    ct[k] = (int8_t)(cos(theta)*127);
    st[k] = (int8_t)(sin(theta)*127);
  }
  
  for (int16_t i=-160; i<160; i++)
  {
    for (int16_t j=-120; j<120; j++)
    {
      uint16_t s = 0.f;
      for (int k=0; k<5; k++) {
        s += (uint16_t)( ct[k]*i+st[k]*j+shift >> 7) & 31;
      }
      //s = s*177/160;
      uint16_t color = 0;
      color += (((uint16_t)(s))&0b111110000)>>4;
      color = (color<<11) | (color<<5) | (color) ;
      write_data(color);
    }
  }
  
  shift+=4;
}  


void loop() {
  stripe();
}


