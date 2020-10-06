#ifndef _Arduino_TFTLCD_H_
#define _Arduino_TFTLCD_H_
#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include "Arduino_GFX.h"   // Adafruits library (slightly modified)
#include "colors.h"    // Hard coded "fast" colors
#include "registers.h" // LCD driver register names
#include "delays.h"    // Assembly commands for inserting delays 

#if defined(__AVR_ATmega32U4__)
    // Using Leonardo. Import Leonardo pin definitions. 
    // Convert most macros to functions to save space.
    // This will be slow
    #include "Leonardo_TFT.h"
    #define SAVE_SPACE
#else 
    // Using Uno. This will be fast. Import Uno pin definitions.
    // Use macro expansions aggressively
    #include "Uno_TFT.h"
    //#define SAVE_SPACE
#endif

// The leonardo doesn't have very much space on it.
// We have to convert some of the macros to function calls.
#ifdef SAVE_SPACE
void START_PIXEL_DATA();
void COMMAND(uint8_t CMD);
void SEND_PAIR(uint8_t hi,uint8_t lo);
void SEND_PERMUTED_PAIR(uint8_t hi,uint8_t lo);
void START_READING();
void STOP_READING();
void SET_XY_RANGE(uint8_t x0,uint8_t x1,uint16_t y0);
void SET_XY_LOCATION(uint8_t x,uint16_t y);
void SET_X_LOCATION(uint8_t x);
void SET_Y_LOCATION(uint16_t y);
void RESET_X_RANGE();
void ZERO_XY();
#endif


//////////////////////////////////////////////////////////////////////////
// Platform independent macros

#include "TFT_macros.h"

class Arduino_TFTLCD : public Arduino_GFX {

 public:

  Arduino_TFTLCD(uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t rst);
  Arduino_TFTLCD(void);
  
  // Masked rendering is used to erase regions to background, skipping
  // pixels from the current frame.
  // Overdraw rendering is used to avoid redrawing parts of the current
  // frame that have already been rendered. 
  
  // State variables for controlling masked and overdrawn rendering
  uint8_t  mask_flag = 0;
  uint8_t  do_masking = 0;  
  uint8_t  do_overdraw = 0;  
  uint16_t background_color = 0;
  uint16_t foreground_color = WHITE;
  
  // Functions for controlling masked and overdrawn rendering
  void     overdraw_on();
  void     overdraw_off();
  void     masking_on();
  void     masking_off();
  void     flip_mask();
  
  // "accurate" Drawing routines
  void     drawPixel(int16_t x, int16_t y, uint16_t color);
  void     drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void     drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
  void     drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
  void     fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void     fillScreen(uint16_t color);
  
  // System configuration
  void     begin();
  void     setRegisters8(uint8_t *ptr, uint8_t n);
  void     setRegisters16(uint16_t *ptr, uint8_t n);
  void     set_low_color_mode(uint8_t ison);

  // Functions for reading color data
  uint16_t readPixel(int16_t x, int16_t y);

  // Fast drawing routines. No clipping, reduced color accuracy.
  void     fastFlood(uint8_t color, uint16_t length);
  void     fastPixel(uint8_t x, uint16_t y, uint8_t color);
  void     fastLine(uint8_t x0, uint16_t y0, uint8_t x1, uint16_t y1, uint8_t color);
  void     fastFillTriangle(uint8_t x0, uint16_t y0, uint8_t x1, uint16_t y1, uint8_t x2, uint16_t y2, uint8_t color);
  void     fastDrawTriangle(uint8_t x0, uint16_t y0, uint8_t x1, uint16_t y1, uint8_t x2, uint16_t y2, uint8_t color);
  void     fastestHLine(uint8_t x0, uint16_t y0, uint16_t w, uint8_t color);
  void     fastestVLine(uint8_t x0, uint16_t y0, uint16_t h, uint8_t color);
  void     fastFillRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c);
  void     fastDrawRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c);
  void     fastFillScreen(uint8_t color);
  void     fastXORFlood(uint8_t mask, uint8_t length);
  void     fastXORRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t mask);

 private:
  void     colorPixel(uint16_t y, uint16_t permuted_color);
  void     init(),
           flood(uint16_t color, uint32_t len);

};



#endif
