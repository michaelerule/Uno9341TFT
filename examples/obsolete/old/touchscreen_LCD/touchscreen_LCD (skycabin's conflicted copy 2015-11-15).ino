#include <SPI.h>
#include <stdint.h>

// Pin definitions as currently hooked up on the ardunio test circuit.
// These should be set up before including the file "chinese_LCD_lib.h"
#define CS    0
#define RESET 1
#define A0_XC 2
#define LCD_SDA  11
#define LCD_SCK  13
#define LED   5


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

void setup() {
  initialize_LCD();
  /*
  uint16_t seq = 0;
  while(1) {
    write_command(seq);
    write_command(seq>>8);
    for (uint16_t i=0;i<0x0fff; i+=8) {
      write_data(i);
      write_data(i>>8);
    }
    seq++;
  }
  */
  write_command(START_FRAME); 
  clear_background();
  //write_command(START_FRAME); 
  //set_background(0b111);
}

void clear_background() {
  digitalWrite(LCD_RS,HIGH);
  digitalWrite(LCD_CS,LOW);    

  digitalWrite(LCD_D0,0);
  digitalWrite(LCD_D1,0);
  digitalWrite(LCD_D2,0);
  digitalWrite(LCD_D3,0);
  digitalWrite(LCD_D4,0);
  digitalWrite(LCD_D5,0);
  digitalWrite(LCD_D6,0);
  digitalWrite(LCD_D7,0);

  for (int32_t i=0; i<320*240*2; i++) {
      digitalWrite(LCD_WR,LOW);
      digitalWrite(LCD_WR,HIGH);
  }
  digitalWrite(LCD_CS,HIGH);
}

void set_background(uint16_t color)
{
  for (int16_t i=0; i<320; i++)
    for (int16_t j=0; j<320; j++)
      write_data(color);
}  


uint8_t shift = 0;

void stripe()
{
  int k;
  k=0; 
  for (uint16_t i=0; i<320; i++)
  {
    for (uint16_t j=0; j<240; j++)
    {
      uint16_t color = 0;
      color += (((i+j+shift>>1))&0b111110000)>>4;
      color = (color<<11) | (color<<5) | (color) ;
      write_data_fast(color);
    }
  }
  shift-=2;
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
  //write_command(START_FRAME); 
  //stripe();
  //quasi();
  //write_command(START_FRAME); 
  //set_background(0x00F0);
}


