#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "Uno9163_TFT.h"
#include "pin_magic.h"
#define TFTWIDTH   128
#define TFTHEIGHT  128
// LCD controller chip identifiers
#define ID_9341    2
#define ID_UNKNOWN 0xFF
#include "registers.h"

// Turn this on to activate software clipping to screen bounds
//#define DO_CLIP

Uno9163_TFT::Uno9163_TFT(
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
Uno9163_TFT::Uno9163_TFT(void) : Uno_GFX(TFTWIDTH, TFTHEIGHT) {
  init();
}

// Initialization common to both shield & breakout configs
void Uno9163_TFT::init(void) {
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

void Uno9163_TFT::begin(uint16_t id) {
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
}

void Uno9163_TFT::reset(void) {
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
inline void Uno9163_TFT::setAddrWindow(int x1, int y1, int x2, int y2) 
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
void Uno9163_TFT::setLR(void) {
    RESET_X_RANGE;
    ZERO_Y;
}

/*
 Fast block fill operation for fillScreen, fillRect, H/V line, etc.
 Requires setAddrWindow() has previously been called to set the fill
 bounds.  'len' is inclusive, MUST be >= 1.
 */ 
inline void Uno9163_TFT::flood(uint16_t color, uint32_t len) {
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
void Uno9163_TFT::drawFastHLine(int16_t x, int16_t y, int16_t length,
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

void Uno9163_TFT::drawFastVLine(int16_t x, int16_t y, int16_t length,
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

void Uno9163_TFT::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t fillcolor) {
    int16_t  x2=x1+w-1, y2=y1+h-1;
#ifdef DO_CLIP
    // Initial off-screen clipping
    if( (w          <= 0     ) ||  (h             <= 0      ) ||
      (x1           >= _width) ||  (y1            >= _height) |
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
void Uno9163_TFT::fillScreen(uint16_t color) {
    ZERO_X;
    ZERO_Y;
    flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

uint8_t Uno9163_TFT::readMaskBit(int16_t x, int16_t y) {
    return -1;
}

void Uno9163_TFT::drawPixel(int16_t x0, int16_t y0, uint16_t color) {
    // Clip
#ifdef DO_CLIP
    if((x0<0)||(y0<0)||(x0>=_width)||(y0>=_height)) return;
#endif
    SET_X_LOCATION(x0);
    SET_Y_LOCATION(y0);
    // Draw Pixel data only if mask flag does not match
    CLOCK_COMMAND(BEGIN_PIXEL_DATA);
    CLOCK_DATA(color>>8);
    CLOCK_DATA(color);
}

void Uno9163_TFT::colorPixel(uint16_t color) {
    CLOCK_COMMAND(BEGIN_PIXEL_DATA);
    CLOCK_DATA(color>>8);
    CLOCK_DATA(color);
}


// Bresenham's algorithm - thx wikpedia
void Uno9163_TFT::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
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
void Uno9163_TFT::pushColors(uint16_t *data, uint8_t len, boolean first) {
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

#define read8(x) x=read8fn()

// Because this function is used infrequently, it configures the ports for
// the read operation, reads the data, then restores the ports to the write
// configuration.  Write operations happen a LOT, so it's advantageous to
// leave the ports in that state as a default.
uint16_t Uno9163_TFT::readPixel(int16_t x, int16_t y) {
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
uint8_t Uno9163_TFT::readHigh(int16_t x, int16_t y) {
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
uint16_t Uno9163_TFT::readID(void) {
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

uint32_t Uno9163_TFT::readReg(uint8_t r) {
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

void Uno9163_TFT::write8(uint8_t value) {
  write8inline(value);
}

uint8_t Uno9163_TFT::read8fn(void) {
  uint8_t result;
  read8inline(result);
  return result;
}

void Uno9163_TFT::setWriteDir(void) {
  setWriteDirInline();
}

void Uno9163_TFT::setReadDir(void) {
  setReadDirInline();
}

void Uno9163_TFT::writeRegister8(uint8_t a, uint8_t d) {
  writeRegister8inline(a, d);
}

void Uno9163_TFT::writeRegister16(uint16_t a, uint16_t d) {
  writeRegister16inline(a, d);
}

void Uno9163_TFT::writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d) {
  writeRegisterPairInline(aH, aL, d);
}

void Uno9163_TFT::writeRegister24(uint8_t r, uint32_t d) {
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

void Uno9163_TFT::writeRegister32(uint8_t r, uint32_t d) {
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
