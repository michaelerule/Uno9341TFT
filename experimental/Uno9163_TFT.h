#include "Arduino.h"
#include <Uno_GFX.h>

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




/* Control lines are all on PORTC, and data lines are on PORTB and PORTD.

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

// Pull the CS pin low to enable serial communication
// Pull the A0 pin low to indicate we are sending a command
#define READY_COMMAND {\
    digitalWrite(CS,LOW);\
    digitalWrite(A0_XC,LOW);\
}
#define SEND_COMMAND {digitalWrite(CS,HIGH);}

// Pull the CS pin low to enable serial communication
// Pull the A0 pin high to indicate we are sending data
#define READY_DATA {\
    digitalWrite(CS,LOW);\
    digitalWrite(A0_XC,HIGH);\
}
#define SEND_DATA {digitalWrite(CS,HIGH);}

/*
The most significant bit must be clocked in first.
The pin "CS" must be pulled low to enable serial 
(this function does not manage the CS pin)
The SDA ("serial data") pin is sampled on the rising edge.
*/
#define WRITE_BUS(b) {\
    for (int i=7; i>=0; i--) {\
        digitalWrite(LCD_SDA,(data>>i)&1);\
        digitalWrite(LCD_SCK,LOW);\
        digitalWrite(LCD_SCK,HIGH);\
    }\
}

#define CLOCK_COMMAND(b) {\
    READY_COMMAND;\
    WRITE_BUS(b);\
    SEND_COMMAND;\
}

#define CLOCK_DATA(b) {\
    READY_DATA;\
    WRITE_BUS(b);\
    SEND_DATA;\
}\

/*
 The 9341 and 9163 accepts 1-byte commands. Here we define the commonly aliases for
 the commonly used commands.
 */
#define SET_COLUMN_ADDRESS_WINDOW 0x2A
#define SET_ROW_ADDRESS_WINDOW    0x2B
#define BEGIN_PIXEL_DATA          0x2C
#define BEGIN_READ_DATA           0x2E
#define DISPLAY_ON                0x29
#define DISPLAY_OFF               0x28

// Move the left bound of drawing window to 0
#define ZERO_X {\
    CLOCK_COMMAND(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_DATA(0);\
    CLOCK_DATA(0);\
}

// Move the upper bound of drawing window to 0
#define ZERO_Y {\
    CLOCK_COMMAND(SET_ROW_ADDRESS_WINDOW);\
    CLOCK_DATA(0);\
    CLOCK_DATA(0);\
}

// Set the X location 
// Because x in 0..239, top byte is always 0
#define SET_X_LOCATION(x) {\
    CLOCK_COMMAND(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_DATA(0);\
    CLOCK_DATA(x);\
}

// Set the Y location
#define SET_Y_LOCATION(y) {\
    CLOCK_COMMAND(SET_ROW_ADDRESS_WINDOW);\
    CLOCK_DATA((y)>>8);\
    CLOCK_DATA((y));\
}

// Set X range
#define SET_X_RANGE(x1,x2) {\
    CLOCK_COMMAND(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_DATA(0);\
    CLOCK_DATA(x1);\
    CLOCK_DATA(0);\
    CLOCK_DATA(x2);\
}

// Sets X range to (0,239)
#define RESET_X_RANGE {\
    CLOCK_COMMAND(SET_COLUMN_ADDRESS_WINDOW);\
    CLOCK_DATA(0);\
    CLOCK_DATA(0);\
    CLOCK_DATA(0);\
    CLOCK_DATA(239);\
}

class Uno9163_TFT : public Uno_GFX {
 public:
  Uno9163_TFT(uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t rst);
  Uno9163_TFT(void);
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
  write8(uint8_t value),
  setWriteDir(void),
  setReadDir(void),
  writeRegister8(uint8_t a, uint8_t d),
  writeRegister16(uint16_t a, uint16_t d),
  writeRegister24(uint8_t a, uint32_t d),
  writeRegister32(uint8_t a, uint32_t d),
  writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d),
  setLR(void),
  flood(uint16_t color, uint32_t len);
  uint8_t  driver;
  uint8_t  read8fn(void);
};




#endif
