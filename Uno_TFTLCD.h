// IMPORTANT: SEE COMMENTS @ LINE 15 REGARDING SHIELD VS BREAKOUT BOARD USAGE.

// Graphics library by ladyada/adafruit with init code from Rossum
// MIT license

#ifndef _Uno_TFTLCD_H_
#define _Uno_TFTLCD_H_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Uno_GFX.h>

// **** IF USING THE LCD BREAKOUT BOARD, COMMENT OUT THIS NEXT LINE. ****
// **** IF USING THE LCD SHIELD, LEAVE THE LINE ENABLED:             ****

//#define USE_ADAFRUIT_SHIELD_PINOUT 1

#define color565(r, g ,b) ((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3))


// Assign human-readable names to some common 16-bit color values:
// Using values with identical high and low bytes gives faster
// Rendering of filled regions
#define BLACK   0b0000000000000000
#define BLUE    0b0001001100010011
#define RED     0b1110000011100000
#define GREEN   0b0000011100000111
#define CYAN    0b0001011100010111
#define MAGENTA 0b1111000011110000
#define YELLOW  0b1110011111100111
#define WHITE   0b1111011111110111
#define MASK   ~0b1111011111110111

// Inline assembly commands for inserting very short delays.
// Reading data form the 9341 in particular requires a certain delay
// between when the read command is sent and when the data becomes
// available on the bus line. These delays are used as "shims" to get
// the timing right. They are currently set for a 16MHz clock, and 
// modificatios of the AtMega clock rate will likely required adjusting
// the inline delays used for reading data. ( I'm looking at use over-
// clockers). 
// 
// Pixel read operations require a minimum 400 nS delay from RD_ACTIVE
// to polling the input pins.  At 16 MHz, one machine cycle is 62.5 nS.
// RJMPs are equivalent to two NOPs each, 
// NOP burns one cycle
#define DELAY3        \
  asm volatile(       \
    "rjmp .+0" "\n\t" \
    "nop"      "\n"   \
    ::);
#define DELAY2        \
  asm volatile(       \
    "rjmp .+0" "\n\t" \
    "nop"      "\n"   \
    ::);
#define DELAY1        \
  asm volatile(       \
    "rjmp .+0" "\n\t" \
    "nop"      "\n"   \
    ::);

/*
Control lines are all on PORTC, and data lines are on PORTB and PORTD.

Communication is faster if we write to PORTC directly. There is one unused
GPIO pin that will be clobbered in this operation, so projects using this
pin should plan accordingly. The reset line is also on PORTC so care must
be taken not to trigger it. 

On the 9341, the 8-bit data bus is split with the top six bits on PORTB
and the lower two bits on PORTD. The lower two bits on PORTB are digital 
pins 0 and 1, used for the RX and TX serial lines. As far as I can tell, 
when the UART is enabled on the AtMega, these pins are decoupled from 
PORTB, or at least I have found that setting these pins in the display
driver does not seem to interfere with serial communication. Likewise, 
upper bits on PORTD are used to communicate with the touch screen and SD
card slot and for whatever reason sending spurious data on these lines does
not seem to interfere with these functions. If interference is observed,
simply set critical pins to input mode (high impedence) while running
display driver code. Because of this lack of interference, we can send 
data using the succinct PORTB=PORTD=data.

Bits are as follows on PORTC (there is no PC7 on AtMega*8)
PC6: Reset (keep high)
PC5: Not used,  set to 1 by TFT control code
PC4: Always 1? 
PC3: Always 0?
PC2: Data send bit. 1 for sending data, 0 for sending command
PC1: Write clock bit. Triggered on rising edge, I think. 
PC0: Chip select line. 1 for enabel. Left high by driver code. 

*/
#define READY_COMMAND     0b1110001
#define SEND_COMMAND      0b1110011
#define READY_DATA        0b1110101
#define SEND_DATA         0b1110111
#define REQUEST_READ      0b1110010

#define WRITE_BUS(b)      PORTB=PORTD=(b)

#define CLOCK_COMMAND {\
    PORTC=READY_COMMAND;\
    PORTC=SEND_COMMAND;\
}

#define CLOCK_DATA {\
    PORTC=READY_DATA;\
    PORTC=SEND_DATA;\
}\

/*
 The 9341 accepts 1-byte commands. Here we define the commonly aliases for
 the commonly used commands. 
 */
#define SET_COLUMN_ADDRESS_WINDOW 0x2A
#define SET_ROW_ADDRESS_WINDOW    0x2B
#define BEGIN_PIXEL_DATA          0x2C
#define BEGIN_READ_DATA           0x2E

// Move the left bound of drawing window to 0
#define ZERO_X {\
    WRITE_BUS(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    CLOCK_DATA;\
}

// Move the upper bound of drawing window to 0
#define ZERO_Y {\
    WRITE_BUS(SET_ROW_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    CLOCK_DATA;\
}

// Set the X location 
// Because x in 0..239, top byte is always 0
#define SET_X_LOCATION(x) {\
    WRITE_BUS(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    WRITE_BUS((x));\
    CLOCK_DATA;\
}

// Set the Y location
#define SET_Y_LOCATION(y) {\
    WRITE_BUS(SET_ROW_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS((y)>>8);\
    CLOCK_DATA;\
    WRITE_BUS((y));\
    CLOCK_DATA;\
}

// Set X range
#define SET_X_RANGE(x1,x2) {\
    WRITE_BUS(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    WRITE_BUS((x1));\
    CLOCK_DATA;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    WRITE_BUS((x2));\
    CLOCK_DATA;\
}

// Sets X range to (0,239)
#define RESET_X_RANGE {\
    WRITE_BUS(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_COMMAND;\
    WRITE_BUS(0);\
    CLOCK_DATA;\
    CLOCK_DATA;\
    CLOCK_DATA;\
    WRITE_BUS(239);\
    CLOCK_DATA;\
}

class Uno_TFTLCD : public Uno_GFX {

 public:

  Uno_TFTLCD(uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t rst);
  Uno_TFTLCD(void);
  uint8_t  mask_flag;
  uint8_t  masking_on;  
  void     begin(uint16_t id = 0x9325);
  void     drawPixel(int16_t x, int16_t y, uint16_t color);
  void     colorPixel(uint16_t color);
  void     drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void     drawFastHLine(int16_t x0, int16_t y0, int16_t w, uint16_t color);
  void     drawFastVLine(int16_t x0, int16_t y0, int16_t h, uint16_t color);
  void     fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c);
  void     fillScreen(uint16_t color);
  void     reset(void);
  void     setRegisters8(uint8_t *ptr, uint8_t n);
  void     setRegisters16(uint16_t *ptr, uint8_t n);
       // These methods are public in order for BMP examples to work:
  void     setAddrWindow(int x1, int y1, int x2, int y2);
  void     pushColors(uint16_t *data, uint8_t len, boolean first);

  uint8_t  readHigh(int16_t x, int16_t y);
  
  uint16_t readPixel(int16_t x, int16_t y),
           readID(void);
  uint8_t  readMaskBit(int16_t x, int16_t y);
  uint32_t readRGB(int16_t x, int16_t y);
  uint32_t readReg(uint8_t r);

 private:

  void     init(),
           // These items may have previously been defined as macros
           // in pin_magic.h.  If not, function versions are declared:
#ifndef write8
           write8(uint8_t value),
#endif
#ifndef setWriteDir
           setWriteDir(void),
#endif
#ifndef setReadDir
           setReadDir(void),
#endif
#ifndef writeRegister8
           writeRegister8(uint8_t a, uint8_t d),
#endif
#ifndef writeRegister16
           writeRegister16(uint16_t a, uint16_t d),
#endif
    writeRegister24(uint8_t a, uint32_t d),
    writeRegister32(uint8_t a, uint32_t d),
#ifndef writeRegisterPair
           writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d),
#endif
           setLR(void),
           flood(uint16_t color, uint32_t len);
  uint8_t  driver;

#ifndef read8
  uint8_t  read8fn(void);
  #define  read8isFunctionalized
#endif

#ifndef USE_ADAFRUIT_SHIELD_PINOUT

  #ifdef __AVR__
    volatile uint8_t *csPort    , *cdPort    , *wrPort    , *rdPort;
	uint8_t           csPinSet  ,  cdPinSet  ,  wrPinSet  ,  rdPinSet  ,
					  csPinUnset,  cdPinUnset,  wrPinUnset,  rdPinUnset,
					  _reset;
  #endif
  #if defined(__SAM3X8E__)
    Pio *csPort    , *cdPort    , *wrPort    , *rdPort;
	uint32_t          csPinSet  ,  cdPinSet  ,  wrPinSet  ,  rdPinSet  ,
					  csPinUnset,  cdPinUnset,  wrPinUnset,  rdPinUnset,
					  _reset;
  #endif
  
#endif
};

// For compatibility with sketches written for older versions of library.
// Color function name was changed to 'color565' for parity with 2.2" LCD
// library.
#define Color565 color565

#endif
