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

#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "Uno_TFTLCD.h"
#include "pin_magic.h"
#define TFTWIDTH   240
#define TFTHEIGHT  320
// LCD controller chip identifiers
// ONLY SUPPORTING 9341 DRIVER AT THE MOMENT
#define ID_9341    2
#include "registers.h"

// Turn this on to activate software clipping to screen bounds
// Leave this on something weird is happening without it
//#define DO_CLIP

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS AND SYSTEM ROUTINES
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// Constructor for breakout board (configurable LCD control lines).
// Can still use this w/shield, but parameters are ignored.
// This has not been modified from the original Adafruit implementation
// and it likely contains some dead code as a result. 
Uno_TFTLCD::Uno_TFTLCD(uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t reset) : Uno_GFX(TFTWIDTH, TFTHEIGHT) {
    #ifndef USE_ADAFRUIT_SHIELD_PINOUT
        // Convert pin numbers to registers and bitmasks
        _reset = reset;
        #ifdef __AVR__
            csPort = portOutputRegister(digitalPinToPort(cs));
            cdPort = portOutputRegister(digitalPinToPort(cd));
            wrPort = portOutputRegister(digitalPinToPort(wr));
            rdPort = portOutputRegister(digitalPinToPort(rd));
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
            *csPort |= csPinSet; // Set all control bits to HIGH (idle)
            *cdPort |= cdPinSet; // Signals are ACTIVE LOW
            *wrPort |= wrPinSet;
            *rdPort |= rdPinSet;
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
}

void Uno_TFTLCD::low_color_mode_on() {
    PORTC=0b1110001;
    PORTB=PORTD=0x39;
    PORTC=0b1110011;
    use_low_color_mode=1;
}

void Uno_TFTLCD::low_color_mode_off() {
    PORTC=0b1110001;
    PORTB=PORTD=0x38;
    PORTC=0b1110011;
    use_low_color_mode=0;
}

void Uno_TFTLCD::toggle_low_color_mode() {
    if (use_low_color_mode) low_color_mode_off();
    else low_color_mode_on();
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

/* Send command to 9341. */
void write_command(uint16_t command)
{
    WRITE_BUS(command>>8); CLOCK_COMMAND;
    WRITE_BUS(command);    CLOCK_COMMAND;
}

/* Send data to 9341. */ 
void write_data(uint16_t data) {
    SEND_PAIR(data>>8,data);
}

/*
Sets the LCD address window (and address counter, on 932X).
Relevant to rect/screen fills and H/V lines.  Input coordinates are
assumed pre-sorted (e.g. x2 >= x1).
This does not set the upper bounds for the row information. This is left
at the initialization value of 319. There is no reason to ever change this
register. 
*/
void Uno_TFTLCD::setAddrWindow(int x1, int y1, int x2, int y2) 
{
    SET_XY_RANGE(x1,x2,y1);
}

/*
In order to save a few register writes on each pixel drawn, the lower-right
corner of the address window is reset after most fill operations, so that 
drawPixel only needs to change the upper left each time.
The row range is set to 0..239
The col range is set to 0..Current
Transmission of the end of the column range command is ommitted, as this
should be set to 319 during initialization and is never changed by the
driver code. As long as no other code sends data afterwards, this register
will remain set to its original value. This driver does not send data
without issuing a command first, so this works. However, it could interact
poorly with user code if the chip select line for the touch screen is not
disabled prior to performing IO operations on PORTC. 
*/
void Uno_TFTLCD::setLR(void) {
    RESET_X_RANGE();
    ZERO_Y();
}

////////////////////////////////////////////////////////////////////////////
// BASIC DRAWING ROUTINES
////////////////////////////////////////////////////////////////////////////

void Uno_TFTLCD::fastFlood(uint8_t c, uint16_t l) {
    flood( 0x0101*c, l);
}

// Very fast flood routine. 
void Uno_TFTLCD::flood(uint16_t color, uint32_t len) {
    uint8_t  i;
    uint8_t hi = color >> 8;
    uint8_t lo = color;
    START_PIXEL_DATA();
    
    if(hi == lo) {
        PORTB=PORTD=color;
        while (len>=64) {
            CLOCK_64;
            len-=64;
        } 
        if (len &0b00100000) { CLOCK_32; }
        if (len &0b00010000) { CLOCK_16; }
        if (len &0b00001000) { CLOCK_8; }
        if (len &0b00000100) { CLOCK_4; }
        if (len &0b00000010) { CLOCK_2; }
        if (len &0b00000001) { CLOCK_1; }
    } else {
        uint16_t blocks = (uint16_t)(len/32); 
        while(blocks--) {
          i = 4;
          do {
            SEND_PAIR(hi,lo); SEND_PAIR(hi,lo); 
            SEND_PAIR(hi,lo); SEND_PAIR(hi,lo);
            SEND_PAIR(hi,lo); SEND_PAIR(hi,lo); 
            SEND_PAIR(hi,lo); SEND_PAIR(hi,lo);
          } while(--i);
        }
        for(i = (uint8_t)len&31; i--; ) SEND_PAIR(hi,lo);
    }
}

void Uno_TFTLCD::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t fillcolor) {
    int16_t  x2=x1+w-1, y2=y1+h-1;
    #ifdef DO_CLIP
        if(w<=0||h<=0||x1>=_width||y1>=_height||x2<0||y2<0) return;
        if(x1<0) {w+=x1;x1=0;}
        if(y1<0) {h+=y1;y1=0;}
        if(x2>=_width) {x2=_width-1;w=x2-x1+1;}
        if(y2>=_height){y2=_height-1;h=y2-y1+1;}
    #endif
    SET_XY_RANGE(x1,x2,y1);
    flood(fillcolor, (uint32_t)w * (uint32_t)h);
    RESET_X_RANGE();
}

void Uno_TFTLCD::fillScreen(uint16_t color) {
    ZERO_XY();
    flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

void Uno_TFTLCD::colorPixel(uint16_t y, uint16_t color) {
    uint8_t line_flag = 0b1000*(y&1);
    uint8_t mask_test = mask_flag^line_flag;
    if (do_masking) {
        START_READING();
        DELAY1
        uint8_t R = PIND;
        STOP_READING();
        if ((R&0b1000)==mask_test) return;
    } else {
        color &= 0b1111011111111111;
        color |= (uint16_t)(mask_flag^line_flag)<< 8;
    }
    START_PIXEL_DATA();
    SEND_PIXEL(color);
}

void Uno_TFTLCD::drawPixel(int16_t x, int16_t y, uint16_t color) {
    #ifdef DO_CLIP
        if((x<0)||(y<0)||(x>=_width)||(y>=_height)) return;
    #endif
    SET_XY_LOCATION(x,y);
    colorPixel(y,color);
}

void Uno_TFTLCD::drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
    #ifdef DO_CLIP
        int16_t y2=y+length-1;
        if(length<=0||x<0||x>=_width||y>=_height||y2<0) return;
        if(y<0) {length+=y;y=0;}
        if(y2>=_height) {y2=_height-1;length=y2-y+1;}
    #endif
    SET_XY_RANGE(x,x,y);
    flood(color, length);
    RESET_X_RANGE();
}


// When drawing triangles, large contiguous areas will 
// be masked out. So instead we store a list of offsets and
// lengths that are /not/ masked out, and just draw those
// To do this, we start reading the color data. If it is masked, 
// we continue until it is not masked, and mark that position.
// We keep reading unmasked data until we come to a masked pixel,
// or are at the end of the line. We then draw the pixel data.
// We use the continue read data to pick up where we left off.
void Uno_TFTLCD::drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color){
    if (length<1) return;
    #ifdef DO_CLIP
        int16_t x2 = x+length-1;
        if(length<=0||y<0||y>=_height||x>=_width||x2<0) return;
        if(x<0) {length+=x; x=0;}
        if(x2>=_width) {x2=_width-1; length=x2-x+1;}
    #endif
    uint8_t line_flag = 0b1000*(y&1);
    uint8_t mask_test = mask_flag^line_flag;
    uint8_t background_mask = (background_color>>8) & 0b11110100;
    if (!do_masking) {
        color &= 0b1111011111111111;
        color |= (uint16_t)(mask_flag^line_flag)<< 8;
    }
    color >>= 8;
    SET_Y_LOCATION(y);
    if (do_masking || do_overdraw) {
        int in_segment=0;
        int start=x;
        int stop =x+length;
        int i=x;
        while (i<stop) {
            SET_X_LOCATION(i);
            START_READING();
            while (i<stop) {
                uint8_t read = PIND;
                uint8_t is_masked = (read&0b11110100)!=background_mask && (read&0b1000)==mask_test;
                PORTC=SEND_DATA; 
                PORTC=READY_READ;
                PORTC=SEND_DATA; 
                if (is_masked) {
                    if (in_segment) {
                        STOP_READING();
                        SET_X_LOCATION(start);
                        fastFlood(color,i-start);
                        in_segment=0;
                        start=i;
                        i++;
                        break;
                    }
                }
                else if (!in_segment) {
                    start = i;
                    in_segment = 1;
                }     
                PORTC=READY_READ;
                i++;
            }
        }
        STOP_READING();
        if (in_segment) {
            SET_X_LOCATION(start);
            fastFlood(color,i-start);
        }
    } else {
        SET_X_LOCATION(x);
        fastFlood(color,length);
    }
}


void Uno_TFTLCD::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)   { swap(x0, y0); swap(x1, y1); }
    if (x0 > x1) { swap(x0, x1); swap(y0, y1); }
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
            colorPixel(x0,color);
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
            colorPixel(y0,color);
            err -= dy;
            if (err < 0) {
                y0 += ystep;
                SET_Y_LOCATION(y0);
                err += dx;
            }
        }
    }
}  


////////////////////////////////////////////////////////////////////////////
// ROUTINES FOR MASKING AND OVERDRAW
////////////////////////////////////////////////////////////////////////////

// Functions for controlling masked and overdrawn rendering
void     Uno_TFTLCD::overdraw_on()  {do_overdraw = 1;}
void     Uno_TFTLCD::overdraw_off() {do_overdraw = 0;}
void     Uno_TFTLCD::masking_on()   {do_masking  = 1;}
void     Uno_TFTLCD::masking_off()  {do_masking  = 0;}
void     Uno_TFTLCD::flip_mask()    {mask_flag  ^= 0b1000;}

////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL DATA IO ROUTINES
////////////////////////////////////////////////////////////////////////////

#ifdef read8isFunctionalized
  #define read8(x) x=read8fn()
#endif

uint16_t Uno_TFTLCD::readPixel(int16_t x, int16_t y) {
    SET_XY_LOCATION(x,y);
    uint16_t RGB=0;
    PORTC=READY_COMMAND;
    WRITE_BUS(BEGIN_READ_DATA);
    PORTC=REQUEST_READ; //Clock in command and set the RD clock low
    DDRD=0;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=READY_READ; // Read clock low
    DELAY3
    uint8_t R = PIND;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=READY_READ; // Read clock low
    DELAY3
    RGB |= ((uint16_t)(R&B11111000)<<8);
    uint8_t G = PIND;
    PORTC=SEND_DATA; // Read clock rising edge
    PORTC=READY_READ; // Read clock low
    RGB |= ((uint16_t)(G&B11111100)<<3);
    DELAY3
    RGB |= PIND>>3;
    DDRD=~0;
    return RGB;
}

// For I/O macros that were left undefined, declare function
// versions that reference the macros just once:

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

void Uno_TFTLCD::writeRegister8(uint8_t a, uint8_t d) {
  writeRegister8inline(a, d);
}
void Uno_TFTLCD::writeRegister16(uint16_t a, uint16_t d) {
  writeRegister16inline(a, d);
}

#ifndef writeRegisterPair
void Uno_TFTLCD::writeRegisterPair(uint8_t aH, uint8_t aL, uint16_t d) {
  writeRegisterPairInline(aH, aL, d);
}
#endif


////////////////////////////////////////////////////////////////////////////
// Fast drawing extensions.
// Support only a limited color pallet
////////////////////////////////////////////////////////////////////////////


/** X-ORs Pixel data with mask
 *  Fast mode: only top 6 bits of first byte of color data is considered
 *  The High and Low bytes of color data are duplicated.  
 */
void Uno_TFTLCD::fastXORFlood(uint8_t mask, uint8_t length) {
    // bless avr-gcc for supporting variable length arrays on the stack
    uint8_t colors[length];
    // First read the pixels
    START_READING();       
    for(uint16_t i=0; i<length; i++) {
        uint8_t read = PIND;
        PORTC=SEND_DATA;
        PORTC=READY_READ;
        PORTC=SEND_DATA;
        colors[i] = read^mask;
        PORTC=READY_READ;
    }
    STOP_READING();
    START_PIXEL_DATA();
    for(uint16_t i=0; i<length; i++) {
        PORTD=colors[i];
        CLOCK_1;
    }
}

void Uno_TFTLCD::fastFillScreen(uint8_t color) {
    ZERO_XY();
    START_PIXEL_DATA();
    PORTD=color;
    for (uint16_t i=0; i<300; i++) CLOCK_256; 
}

void Uno_TFTLCD::fastPixel(uint8_t x, uint16_t y, uint8_t color) {
    SET_XY_LOCATION(x,y);
    START_PIXEL_DATA();
    PORTD=color;
    CLOCK_1;
}

void Uno_TFTLCD::fastXORRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t mask) {
    uint8_t  x2=x+w-1;
    SET_X_RANGE(x,x2);
    for (int i=0; i<h; i++) {
        SET_Y_LOCATION(i+y);
        fastXORFlood(mask,w);
    }
    RESET_X_RANGE();
}

void Uno_TFTLCD::fastFillRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c) {
    uint8_t  x2=x+w-1;
    SET_XY_RANGE(x,x2,y);
    fastFlood(c,w*h);
    RESET_X_RANGE();
    /*    uint16_t y2=y+h;
    for (int i=y; i<y2; i++) 
        fastestHLine(x,i,w,c); */
}

void Uno_TFTLCD::fastDrawRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c) {
    fastestHLine(x, y, w, c);
    fastestHLine(x, y+h-1, w, c);
    fastestVLine(x, y, h, c);
    fastestVLine(x+w-1, y, h, c);
}

void Uno_TFTLCD::fastestVLine(uint8_t x, uint16_t y, uint16_t h, uint8_t color) {
    SET_XY_RANGE(x,x,y);
    fastFlood(color,h);
    RESET_X_RANGE();
}

void Uno_TFTLCD::fastestHLine(uint8_t x, uint16_t y, uint16_t w, uint8_t color) {
    if (!do_masking) {
        uint8_t line_flag = 0b1000*(y&1);
        color &= 0b11110111;
        color |= mask_flag^line_flag;
    }
    SET_XY_LOCATION(x,y);
    fastFlood(color,w);
}

void Uno_TFTLCD::fastDrawTriangle(
    uint8_t x0, uint16_t y0, 
    uint8_t x1, uint16_t y1, 
    uint8_t x2, uint16_t y2, uint8_t color) {
  fastLine(x0, y0, x1, y1, color);
  fastLine(x1, y1, x2, y2, color);
  fastLine(x2, y2, x0, y0, color);
}

void Uno_TFTLCD::fastFillTriangle(
    int16_t x0, int16_t y0, 
    int16_t x1, int16_t y1, 
    int16_t x2, int16_t y2, uint8_t color) {
    int16_t a, b, y;
    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); }
    if (y1 > y2) { swap(y2, y1); swap(x2, x1); }
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); }
    if(y0 == y2)  return;
    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;
    int16_t last = y1==y2? y1 : y1-1; 
    sa += dx01;
    sb += dx02;
    for(y=y0+1; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if(a > b) swap(a,b);
        fastestHLine(a, y, b-a, color);
    }
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if(a > b) swap(a,b);
        fastestHLine(a, y, b-a, color);
    }
}


// fastest way to draw a line. 
// Sacrifices color accuracy for speed.
// Does not support masking.
void Uno_TFTLCD::fastLine(
    uint8_t x0, uint16_t y0, 
    uint8_t x1, uint16_t y1, 
    uint8_t color) {
    uint16_t dx = x1>x0?x1-x0:x0-x1;
    uint16_t dy = y1>y0?y1-y0:y0-y1;
    
    color &= 0b11110111;
    
    if (dy>dx) {
        if (y0 > y1) { 
            swapU16(y0, y1); 
            swapU8(x0, x1); 
        }
        int16_t err = dy/2;
        int16_t xstep = x0<x1?1:-1;
        SET_XY_RANGE(x0,x0,y0);
        START_PIXEL_DATA();
        while (y0<=y1) {
            PORTB=PORTD=color|mask_flag^(0b1000*(y0&1));
            CLOCK_1; 
            y0++;
            err -= dx;
            if (err < 0) {
              x0 += xstep;
              SET_XY_RANGE(x0,x0,y0);
              START_PIXEL_DATA();
              err += dy;
            }
        }
        RESET_X_RANGE();
    } else {
        if (x0 > x1) { 
            swapU16(y0, y1); 
            swapU8(x0, x1); 
        }
        int16_t err = dx/2;
        int16_t ystep = y0<y1?1:-1;
        SET_XY_LOCATION(x0,y0);
        START_PIXEL_DATA();
        PORTB=PORTD=color|mask_flag^(0b1000*(y0&1));
        while (x0<=x1) {
            CLOCK_1;
            err -= dy;
            x0++;
            if (err < 0) {
                y0 += ystep;
                SET_XY_LOCATION(x0,y0);
                START_PIXEL_DATA();
                PORTB=PORTD=color|mask_flag^(0b1000*(y0&1));
                err += dx;
            }
        }
    }
}  


