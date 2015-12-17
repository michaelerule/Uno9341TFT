/*
Optimize graphics driver for 9341 touch screen LCD driver on AtMega chips. 

The Arduino shield may be located on Ebay as of 2015 using search
strings like "2.4 TFT LCD Touch" and costs about $4USD. The bare LCD module
may be found for $3, without the onboard SD card slot. AtMega chips can be
found for at little as $1.50USD. 

The 9341 touch breakout is limited. The pin for synchronizing display 
updates with display refresh is not made available. The AtMega does
not have enough power to update the display in a single frame.
The amount of video RAM on the LCD driver is at least 75x the amount of 
RAM on the AtMega, so double buffering on the microcontroller is impossible.

Despite this, careful optimization of the communication routines, 
combined with rendering tricks to minimize drawing artifacts due to the 
lack of double buffering, can yield good performance graphics suitable 
even for basic animations. 

This file is derived from Adafruit's TFT driver library, with substantial
modifications. It has been customized to work with the Uno 9341 touch 
screen shields. The datasheet for the 9341 is provided with this library. 
Although it has been specialized for the Arduino Uno, in general it will 
work with any AtMega chip or AtMega based arduino board, provided that the 
mapping between arduino pins matches that of the Uno. 

Features: 
    Optimized for fast communication (up to 10x speedups)
    Implemented read pixel data (previously unsupported for 9341)
    Implemented features for advanced rendering:
        Storing and retrieving mask bits from the pixel data itself
            used to partially erase previous frames and for polygon drawing

Drawbacks:
    Depends on the pinout and PORT definitions for the Arduino Uno
    Only supports the 9341 driver
    Screen rotations disabled / removed
    Port write optimizations mean that spurious interactions with 
        Other peripherals may occur in more advanced setups, although 
        so far this has not emerged as a problem in testing
        
This file is derived from Adafruit's driver. Please support Adafruit by
purchasing their products. Please respect their licensing statements.

In general, I have not cleaned this code or prepared it for release. It 
still contains vestigial code related to driving other LCD displays, and
for other microcontroller environments, but it will not operate on these 
platforms. I am releasing thie project before performing a code clean-up.

-- Mrule 2015/12/15

Graphics library by ladyada/adafruit with init code from Rossum
MIT license
*/

#if defined(__SAM3X8E__)
	#include <include/pio.h>
    #define PROGMEM
    #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
    #define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifdef __AVR__
	#include <avr/pgmspace.h>
#endif
#include "pins_arduino.h"
#include "wiring_private.h"
#include "Uno_TFTLCD.h"
#include "pin_magic.h"

#define TFTWIDTH   240
#define TFTHEIGHT  320

// LCD controller chip identifiers
#define ID_9341    2
#define ID_UNKNOWN 0xFF

#include "registers.h"

// Turn this on to activate software clipping to screen bounds
// Leave this on something weird is happening without it
//#define DO_CLIP

// Enable this to use only the fast color pallet. Uses only the top
// 6 bits of color data and forces the high and low bytes to be the same
//#define TURBO_MODE

// Constructor for breakout board (configurable LCD control lines).
// Can still use this w/shield, but parameters are ignored.
Uno_TFTLCD::Uno_TFTLCD(
  uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t reset) :
  Uno_GFX(TFTWIDTH, TFTHEIGHT) {

#ifndef USE_ADAFRUIT_SHIELD_PINOUT
  // Convert pin numbers to registers and bitmasks
  _reset     = reset;
  #ifdef __AVR__
    csPort     = portOutputRegister(digitalPinToPort(cs));
    cdPort     = portOutputRegister(digitalPinToPort(cd));
    wrPort     = portOutputRegister(digitalPinToPort(wr));
    rdPort     = portOutputRegister(digitalPinToPort(rd));
  #endif
  #if defined(__SAM3X8E__)
    csPort     = digitalPinToPort(cs);
    cdPort     = digitalPinToPort(cd);
    wrPort     = digitalPinToPort(wr);
    rdPort     = digitalPinToPort(rd);
  #endif
  csPinSet   = digitalPinToBitMask(cs);
  cdPinSet   = digitalPinToBitMask(cd);
  wrPinSet   = digitalPinToBitMask(wr);
  rdPinSet   = digitalPinToBitMask(rd);
  csPinUnset = ~csPinSet;
  cdPinUnset = ~cdPinSet;
  wrPinUnset = ~wrPinSet;
  rdPinUnset = ~rdPinSet;
  #ifdef __AVR__
    *csPort   |=  csPinSet; // Set all control bits to HIGH (idle)
    *cdPort   |=  cdPinSet; // Signals are ACTIVE LOW
    *wrPort   |=  wrPinSet;
    *rdPort   |=  rdPinSet;
  #endif
  #if defined(__SAM3X8E__)
    csPort->PIO_SODR  |=  csPinSet; // Set all control bits to HIGH (idle)
    cdPort->PIO_SODR  |=  cdPinSet; // Signals are ACTIVE LOW
    wrPort->PIO_SODR  |=  wrPinSet;
    rdPort->PIO_SODR  |=  rdPinSet;
  #endif
  pinMode(cs, OUTPUT);    // Enable outputs
  pinMode(cd, OUTPUT);
  pinMode(wr, OUTPUT);
  pinMode(rd, OUTPUT);
  if(reset) {
    digitalWrite(reset, HIGH);
    pinMode(reset, OUTPUT);
  }
#endif
  init();
}

// Constructor for shield (fixed LCD control lines)
Uno_TFTLCD::Uno_TFTLCD(void) : Uno_GFX(TFTWIDTH, TFTHEIGHT) {
  init();
}

// Initialization common to both shield & breakout configs
void Uno_TFTLCD::init(void) {
#ifdef USE_ADAFRUIT_SHIELD_PINOUT
  CS_IDLE; // Set all control bits to idle state
  WR_IDLE;
  RD_IDLE;
  CD_DATA;
  digitalWrite(5, HIGH); // Reset line
  pinMode(A3, OUTPUT);   // Enable outputs
  pinMode(A2, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode( 5, OUTPUT);
#endif
  setWriteDir(); // Set up LCD data port(s) for WRITE operations
  rotation  = 0;
  cursor_y  = cursor_x = 0;
  textsize  = 1;
  textcolor = 0xFFFF;
  _width    = TFTWIDTH;
  _height   = TFTHEIGHT;
}

void Uno_TFTLCD::begin(uint16_t id) {
    uint8_t i = 0;
    reset();
    delay(200);
    uint16_t a, d;
    driver = ID_9341;
    CS_ACTIVE;
    writeRegister8(ILI9341_SOFTRESET, 0);
    delay(50);
    writeRegister8(ILI9341_DISPLAYOFF, 0);
    writeRegister8(ILI9341_POWERCONTROL1, 0x23);
    writeRegister8(ILI9341_POWERCONTROL2, 0x10);
    writeRegister16(ILI9341_VCOMCONTROL1, 0x2B2B);
    writeRegister8(ILI9341_VCOMCONTROL2, 0xC0);
    writeRegister8(ILI9341_MEMCONTROL, ILI9341_MADCTL_MY | ILI9341_MADCTL_BGR);
    writeRegister8(ILI9341_PIXELFORMAT, 0x55);
    writeRegister16(ILI9341_FRAMECONTROL, 0x001B);
    writeRegister8(ILI9341_ENTRYMODE, 0x07);
    writeRegister8(ILI9341_SLEEPOUT, 0);
    delay(150);
    writeRegister8(ILI9341_DISPLAYON, 0);
    delay(500);
  
#ifdef TURBO_MODE  
  // 3 bit color mode
  PORTC=0b1110001;
  PORTB=PORTD=0x39;
  PORTC=0b1110011;
#endif
}

void Uno_TFTLCD::reset(void) {
    CS_IDLE;
    WR_IDLE;
    RD_IDLE;
#ifdef USE_ADAFRUIT_SHIELD_PINOUT
    digitalWrite(5, LOW);
    delay(2);
    digitalWrite(5, HIGH);
#else
    if(_reset) {
        digitalWrite(_reset, LOW);
        delay(2);
        digitalWrite(_reset, HIGH);
    }
#endif
    // Data transfer sync
    CS_ACTIVE;
    CD_COMMAND;
    write8(0x00);
    for(uint8_t i=0; i<3; i++) WR_STROBE; // Three extra 0x00s
    CS_IDLE;
}


/*
Send command to 9341. 
*/
void write_command(uint16_t command)
{
    WRITE_BUS(command>>8);
    CLOCK_COMMAND;
    WRITE_BUS(command);
    CLOCK_COMMAND;
}

/*
Send data to 9341. 
*/ 
inline void write_data(uint16_t data)
{
    WRITE_BUS(data>>8);
    CLOCK_DATA;
    WRITE_BUS(data);
    CLOCK_DATA;
}

/*
Sets the LCD address window (and address counter, on 932X).
Relevant to rect/screen fills and H/V lines.  Input coordinates are
assumed pre-sorted (e.g. x2 >= x1).

This does not set the upper bounds for the row information. This is left
at the initialization value of 319. There is no reason to ever change this
register. 
*/
inline void Uno_TFTLCD::setAddrWindow(int x1, int y1, int x2, int y2) 
{
    SET_X_RANGE(x1,x2);
    SET_Y_LOCATION(y1);
}

/*
In order to save a few register writes on each pixel drawn, the lower-right
corner of the address window is reset after most fill operations, so that 
drawPixel only needs to change the upper left each time.

The row range is set to 0..239
The col range is set to 0..

Transmission of the end of the column range command is ommitted, as this
should be set to 319 during initialization and is never changed by the
driver code. As long as no other code sends data afterwards, this register
will remain set to its original value. This driver does not send data
without issuing a command first, so this works. However, it could interact
poorly with user code if the chip select line for the touch screen is not
disabled prior to performing IO operations on PORTC. 
*/
void Uno_TFTLCD::setLR(void) {
    RESET_X_RANGE;
    ZERO_Y;
}

/*
 Fast block fill operation for fillScreen, fillRect, H/V line, etc.
 Requires setAddrWindow() has previously been called to set the fill
 bounds.  'len' is inclusive, MUST be >= 1.
 */ 
inline void Uno_TFTLCD::flood(uint16_t color, uint32_t len) {
  uint16_t blocks;
  uint8_t  i, hi = color >> 8, lo = color;
  WRITE_BUS(BEGIN_PIXEL_DATA);
  CLOCK_COMMAND;
  // Write first pixel normally, decrement counter by 1
  WRITE_BUS(hi);
  CLOCK_DATA;
  WRITE_BUS(lo);
  CLOCK_DATA;
  
  len--;
  blocks = (uint16_t)(len / 64); // 64 pixels/block
  if(hi == lo) {
    // High and low bytes are identical.  Leave prior data
    // on the port(s) and just toggle the write strobe.
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        CLOCK_DATA; CLOCK_DATA;
        CLOCK_DATA; CLOCK_DATA;
        CLOCK_DATA; CLOCK_DATA;
        CLOCK_DATA; CLOCK_DATA;
      } while(--i);
    }
    // Fill any remaining pixels (1 to 64)
    for(i = (uint8_t)len & 63; i--; ) {        
        CLOCK_DATA;
        CLOCK_DATA;
    }
  } else {
    while(blocks--) {
      i = 16; // 64 pixels/block / 4 pixels/pass
      do {
        WRITE_BUS(hi);
        CLOCK_DATA;
        WRITE_BUS(lo);
        CLOCK_DATA;
        
        WRITE_BUS(hi);
        CLOCK_DATA;
        WRITE_BUS(lo);
        CLOCK_DATA;
        
        WRITE_BUS(hi);
        CLOCK_DATA;
        WRITE_BUS(lo);
        CLOCK_DATA;
        
        WRITE_BUS(hi);
        CLOCK_DATA;
        WRITE_BUS(lo);
        CLOCK_DATA;
      } while(--i);
    }
    for(i = (uint8_t)len & 63; i--; ) {
        WRITE_BUS(hi);
        CLOCK_DATA;
        WRITE_BUS(lo);
        CLOCK_DATA;
    }
  }
  CS_IDLE;
}

/*
 Draw a horizontal line using flood feel. We can leave the lower right 
 bounds of the address window intact. We just need to make the address
 window start at the correct row and column. Therefore, the last two
 bytes corresponding to the upper limits are omitted.
 */
void Uno_TFTLCD::drawFastHLine(int16_t x, int16_t y, int16_t length,
  uint16_t color)
{
    int16_t x2;
#ifdef DO_CLIP
    // Initial off-screen clipping
    if((length<=0)||(y<0)||(y>= _height)||(x>=_width)||((x2=(x+length-1))<0)) 
        return;
    if(x < 0) {        // Clip left
        length += x;
        x       = 0;
    }
    if(x2 >= _width) { // Clip right
        x2      = _width - 1;
        length  = x2 - x + 1;
    }
#endif    
    SET_X_LOCATION(x);
    SET_Y_LOCATION(y);
    flood(color,length);
}

void Uno_TFTLCD::drawFastVLine(int16_t x, int16_t y, int16_t length,
  uint16_t color)
{
    int16_t y2;
#ifdef DO_CLIP
    // Initial off-screen clipping
    if((length<=0)||(x<0)||(x>=_width)||(y>=_height)||((y2=(y+length-1))<0)) 
        return;
    if(y < 0) {         // Clip top
        length+=y;y=0;
    }
    if(y2 >= _height) { // Clip bottom
        y2=_height-1;
        length=y2-y+1;
    }
#endif
    // Need to set Xmin, Ymin, and Xmax
    // This is the only operation that should change Xmax
    // So we explicitly restore it when we are done
    // Set X and Y
    // Leave Ymax at the base of the screen
    SET_X_RANGE(x,x);
    SET_Y_LOCATION(y);
    flood(color, length);
    RESET_X_RANGE;
}

void Uno_TFTLCD::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t fillcolor) {
    int16_t  x2=x1+w-1, y2=y1+h-1;
#ifdef DO_CLIP
    // Initial off-screen clipping
    if( (w          <= 0     ) ||  (h             <= 0      ) ||
      (x1           >= _width) ||  (y1            >= _height) ||
     ((x2 = x1+w-1) <  0     ) || ((y2  = y1+h-1) <  0      )) return;
    if(x1 < 0) { // Clip left
        w += x1;
        x1 = 0;
    }
    if(y1 < 0) { // Clip top
        h += y1;
        y1 = 0;
    }
    if(x2 >= _width) { // Clip right
        x2 = _width - 1;
        w  = x2 - x1 + 1;
    }
    if(y2 >= _height) { // Clip bottom
        y2 = _height - 1;
        h  = y2 - y1 + 1;
    }
#endif
    // Modifies Xmin Xmax and Ymin
    // Remember to restore Xmax
    // Set X bounds and the top of Y
    // Ymax should remain at the end of screen
    SET_X_RANGE(x1,x2);
    SET_Y_LOCATION(y1);
    flood(fillcolor, (uint32_t)w * (uint32_t)h);
    RESET_X_RANGE;
}

/*
 If everything is in order only Xmin and Ymin should need to be reset
 */ 
void Uno_TFTLCD::fillScreen(uint16_t color) {
    ZERO_X;
    ZERO_Y;
    flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

uint8_t Uno_TFTLCD::readMaskBit(int16_t x, int16_t y) {
    SET_X_LOCATION(x);
    SET_Y_LOCATION(y);
    // Send read command    
    PORTC=READY_COMMAND;
    WRITE_BUS(BEGIN_READ_DATA);
    PORTC=REQUEST_READ; //Clock in command and set the RD clock low
    // Set ports to receive
    // We only care about the top 6 bits so we only read PORTD
    DDRD=0;
    DELAY3
    PORTC=SEND_DATA; // Read clock rising edge
    DELAY3
    PORTC=0b1110110; // Read clock low
    DELAY3
    PORTC=SEND_DATA; // Read clock rising edge
    DELAY3
    PORTC=0b1110110; // Read clock low
    // Make this larger if data isn't coming across cleanly
    DELAY3
    DELAY3
    DELAY3
    DELAY3
    uint8_t R = PIND;
    DDRD=~0;
    return R;
}

void Uno_TFTLCD::drawPixel(int16_t x0, int16_t y0, uint16_t color) {
    // Clip
#ifdef DO_CLIP
    if((x0<0)||(y0<0)||(x0>=_width)||(y0>=_height)) return;
#endif
    SET_X_LOCATION(x0);
    SET_Y_LOCATION(y0);
    if (masking_on) {
        // Check mask_flag
        // This is the lowest bit of the red channel
        // Retrieve Pixel Data
        // Send read command    
        WRITE_BUS(BEGIN_READ_DATA);
        CLOCK_COMMAND;
        // Set ports to receive
        // We only care about the top 6 bits so we only read PORTD
        DDRD=0;
        PORTC=0b1110110; // Read clock low (falling edge)
        PORTC=SEND_DATA; // Read clock high (rising edge)
        DELAY1
        PORTC=0b1110110; // Read clock low (falling edge)
        DELAY1
        uint8_t R = PIND; // Get red channel data ( 3rd bit is mask flag )
        DDRD=~0; // Restore PORTD as an output
        if ((R&0b1000)==mask_flag) return;
    }    
    // Draw Pixel data only if mask flag does not match
#ifdef TURBO_MODE
    WRITE_BUS(BEGIN_PIXEL_DATA);
    CLOCK_COMMAND;
    PORTD=color;
    CLOCK_DATA;
    CLOCK_DATA;
#else
    WRITE_BUS(BEGIN_PIXEL_DATA);
    CLOCK_COMMAND;
    WRITE_BUS(color>>8);
    CLOCK_DATA;
    WRITE_BUS(color);
    CLOCK_DATA;
#endif
}


void Uno_TFTLCD::colorPixel(uint16_t color) {
    if (masking_on) {
        WRITE_BUS(BEGIN_READ_DATA);
        CLOCK_COMMAND;
        DDRD=0;
        PORTC=0b1110110; // Read clock low (falling edge)
        PORTC=SEND_DATA; // Read clock high (rising edge)
        DELAY1
        PORTC=0b1110110; // Read clock low (falling edge)
        DELAY1
        uint8_t R = PIND; // Get red channel data ( 3rd bit is mask flag )
        DDRD=~0; // Restore PORTD as an output
        if ((R&0b1000)==mask_flag) return;
    }    
#ifdef TURBO_MODE
    WRITE_BUS(BEGIN_PIXEL_DATA);
    CLOCK_COMMAND;
    PORTD=color;
    CLOCK_DATA;
    CLOCK_DATA;
#else
    WRITE_BUS(BEGIN_PIXEL_DATA);
    CLOCK_COMMAND;
    WRITE_BUS(color>>8);
    CLOCK_DATA;
    WRITE_BUS(color);
    CLOCK_DATA;
#endif
}


// Bresenham's algorithm - thx wikpedia
void Uno_TFTLCD::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }
  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }
  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep;
  ystep = y0<y1?1:-1;
  if (steep) {  
      SET_X_LOCATION(y0);
      for (; x0<=x1; x0++) {
        SET_Y_LOCATION(x0);
        colorPixel(color);
        err -= dy;
        if (err < 0) {
          y0 += ystep;
          SET_X_LOCATION(y0);
          err += dx;
        }
      }
  } else {
      SET_Y_LOCATION(y0);
      for (; x0<=x1; x0++) {
        SET_X_LOCATION(x0);
        colorPixel(color);
        err -= dy;
        if (err < 0) {
          y0 += ystep;
          SET_Y_LOCATION(y0);
          err += dx;
        }
      }
  }  
}



// Issues 'raw' an array of 16-bit color values to the LCD; used
// externally by BMP examples.  Assumes that setWindowAddr() has
// previously been set to define the bounds.  Max 255 pixels at
// a time (BMP examples read in small chunks due to limited RAM).
void Uno_TFTLCD::pushColors(uint16_t *data, uint8_t len, boolean first) {
    uint16_t color;
    uint8_t  hi, lo;
    CS_ACTIVE;
    if(first == true) { // Issue GRAM write command only on first call
        CD_COMMAND;
        write8(0x2C);
    }
    CD_DATA;
    while(len--) {
        color = *data++;
        hi    = color >> 8; // Don't simplify or merge these
        lo    = color;      // lines, there's macro shenanigans
        write8(hi);         // going on.
        write8(lo);
    }
    CS_IDLE;
}

#ifdef read8isFunctionalized
  #define read8(x) x=read8fn()
#endif

// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
uint16_t Uno_TFTLCD::readPixel(int16_t x, int16_t y) {
    SET_X_LOCATION(x);
    SET_Y_LOCATION(y);
    uint16_t RGB=0;
    // Send read command    
    PORTC=READY_COMMAND;
    WRITE_BUS(BEGIN_READ_DATA);
    PORTC=REQUEST_READ; //Clock in command and set the RD clock low
    // Set ports to receive
    // We only care about the top 6 bits so we only read PORTD
    DDRD=0;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=0b1110110; // Read clock low
    // Make this larger if data isn't coming across cleanly
    DELAY3
    DELAY3
    uint8_t R = PIND;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=0b1110110; // Read clock low
    // Make this larger if data isn't coming across cleanly
    DELAY3
    DELAY3
    RGB |= ((uint16_t)(R&B11111000)<<8);
    uint8_t G = PIND;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=0b1110110; // Read clock low
    RGB |= ((uint16_t)(G&B11111100)<<3);
    // May want to add a delay here   
    DELAY3
    RGB |= PIND>>3;
    //PORTC=SEND_DATA; // Read clock rising edge
    DDRD=~0;
    return RGB;
}


// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
uint8_t Uno_TFTLCD::readHigh(int16_t x, int16_t y) {
    SET_X_LOCATION(x);
    SET_Y_LOCATION(y);    
    // Send read command    
    WRITE_BUS(BEGIN_READ_DATA);
    CLOCK_COMMAND;
    // Set ports to receive
    // We only care about the top 6 bits so we only read PORTD
    DDRD=0;
    PORTC=0b1110110; // Read clock low (falling edge)
    PORTC=SEND_DATA; // Read clock high (rising edge)
    DELAY1
    PORTC=0b1110110; // Read clock low (falling edge)
    DELAY1
    uint8_t R = PIND; // Get red channel data ( 3rd bit is mask flag )
    DDRD=~0; // Restore PORTD as an output
    return R;
}

// Ditto with the read/write port directions, as above.
uint16_t Uno_TFTLCD::readID(void) {
    uint8_t hi, lo;
    uint16_t id = readReg(0xD3);
    if (id == 0x9341) return id;
    CS_ACTIVE;
    CD_COMMAND;
    write8(0x00);
    WR_STROBE;     // Repeat prior byte (0x00)
    setReadDir();  // Set up LCD data port(s) for READ operations
    CD_DATA;
    read8(hi);
    read8(lo);
    setWriteDir();  // Restore LCD data port(s) to WRITE configuration
    CS_IDLE;
    id = hi; id <<= 8; id |= lo;
    return id;
}

uint32_t Uno_TFTLCD::readReg(uint8_t r) {
  uint32_t id;
  uint8_t x;
  // try reading register #4
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  setReadDir();  // Set up LCD data port(s) for READ operations
  CD_DATA;
  delayMicroseconds(50);
  read8(x);
  id = x;  
  id <<= 8;
  read8(x);
  id  |= x;
  id <<= 8;
  read8(x);
  id  |= x;
  id <<= 8;
  read8(x);
  id  |= x;
  CS_IDLE;
  setWriteDir();  // Restore LCD data port(s) to WRITE configuration
  return id;
}

// For I/O macros that were left undefined, declare function
// versions that reference the inline macros just once:

#ifndef write8
void Uno_TFTLCD::write8(uint8_t value) {
  write8inline(value);
}
#endif

#ifdef read8isFunctionalized
uint8_t Uno_TFTLCD::read8fn(void) {
  uint8_t result;
  read8inline(result);
  return result;
}
#endif

#ifndef setWriteDir
void Uno_TFTLCD::setWriteDir(void) {
  setWriteDirInline();
}
#endif

#ifndef setReadDir
void Uno_TFTLCD::setReadDir(void) {
  setReadDirInline();
}
#endif

#ifndef writeRegister8
void Uno_TFTLCD::writeRegister8(uint8_t a, uint8_t d) {
  writeRegister8inline(a, d);
}
#endif

#ifndef writeRegister16
void Uno_TFTLCD::writeRegister16(uint16_t a, uint16_t d) {
  writeRegister16inline(a, d);
}
#endif

#ifndef writeRegisterPair
void Uno_TFTLCD::writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d) {
  writeRegisterPairInline(aH, aL, d);
}
#endif


void Uno_TFTLCD::writeRegister24(uint8_t r, uint32_t d) {
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  delayMicroseconds(10);
  write8(d >> 16);
  delayMicroseconds(10);
  write8(d >> 8);
  delayMicroseconds(10);
  write8(d);
  CS_IDLE;

}

void Uno_TFTLCD::writeRegister32(uint8_t r, uint32_t d) {
  CS_ACTIVE;
  CD_COMMAND;
  write8(r);
  CD_DATA;
  delayMicroseconds(10);
  write8(d >> 24);
  delayMicroseconds(10);
  write8(d >> 16);
  delayMicroseconds(10);
  write8(d >> 8);
  delayMicroseconds(10);
  write8(d);
  CS_IDLE;

}
