#include <math.h>
#include <Uno_TFTLCD.h> // Hardware-specific library

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
  fast_color_test();
}

// Test the masked HLINE functionality
void loop() {
}

/* Colors with identical high and low bytes can be 
 * written slightly faster to the device. Masked drawing
 * is done using the lowest bit of the red channel, which
 * also affects the high bit of the blue channel if the
 * high and low bytes must be identical. Colors can be 
 * interpolated faster if the byte values are numerically
 * consecutive. Here we investigate a range of fast color
 * pallets. 
 */
void fast_color_test() {
  for (uint16_t low_bits = 0; low_bits < 8; low_bits ++) {
    for (uint16_t mask_bit = 0; mask_bit < 2; mask_bit ++) {
      for (uint16_t high_bits =0; high_bits<16; high_bits +=2) {
        uint16_t color = low_bits | 0b1000*mask_bit | (high_bits<<4); 
        color |= color<<8;
        //color &= 0b1111110011111100;
        tft.fillRect(low_bits*240/8+mask_bit*10,high_bits*240/16,10,240/16,color);
      }
    }
  }
  for (uint16_t c=0; c<8; c++) {
    for (uint16_t mask_bit = 0; mask_bit < 2; mask_bit ++) {
      uint16_t color = (c*0b100001|mask_bit*0b1000) * 0x0101;
      //color &= 0b1111110011111100;
      tft.fillRect(c*240/8+mask_bit*10,240,10,240/16,color);
    }
  }

}

