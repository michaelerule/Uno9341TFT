

#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include "wiring_private.h"
#include "Arduino_TFTLCD.h"

#define TFTWIDTH   240
#define TFTHEIGHT  320

// If there's space, we can turn on software clipping to the screen
// boundaries. This isn't essential, and the fast drawing routines
// don't use it, but it could be nice in some cases. 
#ifndef SAVE_SPACE
    #define DO_CLIP
#endif 

// Set this to disable masking and overdraw
// It will break 3D animations but will make some rendering faster, and 
// improve color rendering accuracy
//#define DISABLE_MASKING_AND_OVERDRAW

// The leonardo doesn't have very much space on it.
// We have to convert some of the macros to function calls, sadly.
#ifdef SAVE_SPACE
void COMMAND(uint8_t CMD) {_COMMAND(CMD);}
void START_PIXEL_DATA() {_START_PIXEL_DATA();}
void SEND_PAIR(uint8_t hi,uint8_t lo) {_SEND_PAIR(hi,lo);}
void SEND_PERMUTED_PAIR(uint8_t hi,uint8_t lo) {_SEND_PERMUTED_PAIR(hi,lo);}
void START_READING() {_START_READING();}
void STOP_READING() {_STOP_READING();}
void SET_XY_RANGE(uint8_t x0,uint8_t x1,uint16_t y0) {_SET_XY_RANGE(x0,x1,y0);}
void SET_XY_LOCATION(uint8_t x,uint16_t y) {_SET_XY_LOCATION(x,y);}
void SET_X_LOCATION(uint8_t x) {_SET_X_LOCATION(x);}
void SET_Y_LOCATION(uint16_t y) {_SET_Y_LOCATION(y);}
void RESET_X_RANGE() {_RESET_X_RANGE();}
void ZERO_XY() {_ZERO_XY();}
#endif

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS AND SYSTEM ROUTINES
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

/**
 * Constructor for shield (fixed LCD control lines)
 */
Arduino_TFTLCD::Arduino_TFTLCD(void) : Arduino_GFX(TFTWIDTH, TFTHEIGHT) {
    pinMode(A3, OUTPUT);    // Enable outputs
    pinMode(A2, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(A0, OUTPUT);
    // reset
    digitalWrite(A4, HIGH);
    pinMode(A4, OUTPUT);
    init();
}

/**
 * Initialization common to both shield & breakout configs
 */
void Arduino_TFTLCD::init(void) {
    setWriteDir(); // Set up LCD data port(s) for WRITE operations
    rotation  = 0;
    cursor_y  = cursor_x = 0;
    textsize  = 1;
    textcolor = 0xFFFF;
    _width    = TFTWIDTH;
    _height   = TFTHEIGHT;
}
  
/**  
 * Initialization commands stored as a table in PROGMEM (this saves space)
 */
#define DELAY_CODE 0
#define NCOMMANDS 11*3+2*6
PROGMEM const uint8_t initialization_commands[NCOMMANDS] = {
    DELAY_CODE           , 255,
    DELAY_CODE           , 255,
    ILI9341_SOFTRESET    , 0x00, 0x00,
    DELAY_CODE           , 255,
    ILI9341_DISPLAYOFF   , 0x00, 0x00,
    ILI9341_POWERCONTROL1, 0x23, 0x00,
    ILI9341_POWERCONTROL2, 0x10, 0x00,
    ILI9341_VCOMCONTROL1 , 0x2B, 0x2B,
    ILI9341_VCOMCONTROL2 , 0xC0, 0x00,
    ILI9341_MEMCONTROL   , ILI9341_MADCTL_MY|ILI9341_MADCTL_BGR, 0x00,
    ILI9341_PIXELFORMAT  , 0x55, 0x00,
    ILI9341_FRAMECONTROL , 0x00, 0x1B,
    ILI9341_SLEEPOUT     , 0x00, 0x00,
    DELAY_CODE           , 255,
    ILI9341_DISPLAYON    , 0x00, 0x00,
    DELAY_CODE           , 255,
    DELAY_CODE           , 255};

/**
 * Retrieve command from the `initialization_commands` list
 */
inline uint8_t get_init_command(uint8_t i) {
    return (uint8_t)pgm_read_byte(&initialization_commands[i]);
}

/**
 * Send a byte of data over the 8-bit serial bus to the TFT display driver
 */
void send_byte(uint8_t byte) {
    WRITE_BUS(byte);
    CLOCK_DATA;
}

/**
 * Initialize a new TFT display connection
 */
void Arduino_TFTLCD::begin() {
    ALL_IDLE;
    RS_LOW;
    delay(200);
    RS_HIGH;
    for(uint8_t i=0; i<4; i++) COMMAND(0);
    uint8_t hi,lo,code;
    for (uint16_t i=0; i<NCOMMANDS;) {
        code = get_init_command(i++);
        hi   = get_init_command(i++);
        if (code==DELAY_CODE) delay(hi);
        else {
            COMMAND(code);
            send_byte(hi);
            if (lo = get_init_command(i++)) {
                send_byte(lo);
                CLOCK_DATA;
            }
        }
    }
}

/**
 * Control the low color mode (not yet implemented?)
 *
 * TODO: unsure of how this works, but if there is a native 8-bit color mode
 * of the 9341 TFT displays, then I will feel very silly for the "fast color"
 * hack of using 16 bit colors with identical high and low bytes. 
 * 
 */ 
void Arduino_TFTLCD::set_low_color_mode(uint8_t ison) {
    COMMAND(ison?LOW_COLOR_MODE_ON:LOW_COLOR_MODE_OFF);
}


////////////////////////////////////////////////////////////////////////////
// BASIC DRAWING ROUTINES
////////////////////////////////////////////////////////////////////////////

/**
 * Call the flood routine for a "fast" color. Fast 16-bit colors have identical
 * high and low bytes. 
 * 
 * @param color 8-bit fast color code
 * @param length number of pixels to flood-fill 
 */ 
void Arduino_TFTLCD::fastFlood(uint8_t color, uint16_t length) {
    flood( 0x0101*color, length);
}

/**
 * Very fast flood routine.
 * 
 * TODO: This may render the wrong colors (due to port bit to pin permutation)
 *   if the user passes a color with matching high and low bytes that is *not*
 *   intended to be a "fast" color.
 *
 * @param color 16-bit color code
 * @param len number of pixels to flood-fill
 */
void Arduino_TFTLCD::flood(uint16_t color, uint32_t len) {
    uint8_t  i;
    uint8_t hi = 0xff&(color>>8);
    uint8_t lo = 0xff&(color);
    START_PIXEL_DATA();
    #ifdef SAVE_SPACE
        /*
        hi = BIT_TO_PORT_PERMUTATION(hi);
        lo = BIT_TO_PORT_PERMUTATION(lo);
        do {
            SEND_PERMUTED_PAIR(hi,lo);
            len--;
        } while (len>0);
        */
        hi = BIT_TO_PORT_PERMUTATION(hi);
        WRITE_PERMUTED_BUS(hi);
        while (len>=8) {
            CLOCK_8;
            len-=8;
        } 
        if (len &0b00000100) { CLOCK_4; }
        if (len &0b00000010) { CLOCK_2; }
        if (len &0b00000001) { CLOCK_1; }
    #else
        if(hi == lo) {
            WRITE_BUS(color);
            while (len>=128) {
                CLOCK_128;
                len-=128;
            } 
            if (len &0b01000000) { CLOCK_64; }
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
    #endif
}

/**
 * Fill rectangular region of screen.
 * 
 * No masking or overdraw checking is performed. FRAME_ID_BIT is not set.
 * 
 * Calls the `flood` subroutine, which will automatically optimize color filling
 * if the low and high bytes of the color data are identical.
 *
 * @param x horizontal coordinate of start of rectangular regi on
 * @param y vertical coordinate of start of rectangular region
 * @param w width of rectangular region (assumes dimension <=240)
 * @param h height of rectangular region
 * @param color 16-bit 565 RGB color code
 */
void Arduino_TFTLCD::fillRect(int16_t x1, int16_t y1, int16_t w, int16_t h, 
  uint16_t color) {
    int16_t  x2=x1+w-1, y2=y1+h-1;
    #ifdef DO_CLIP
        // Clipping draw commands to ensure they lie within the display area
        // takes time. If the user is willing to guarantee that all drawing
        // commands are in-bounds, clipping can be disabled by unsetting the
        // compile-time flag DO_CLIP
        if(w<=0||h<=0||x1>=_width||y1>=_height||x2<0||y2<0) return;
        if(x1<0) {w+=x1;x1=0;}
        if(y1<0) {h+=y1;y1=0;}
        if(x2>=_width ) {x2=_width -1;w=x2-x1+1;}
        if(y2>=_height) {y2=_height-1;h=y2-y1+1;}
    #endif
    SET_XY_RANGE(x1,x2,y1);
    flood(color, (uint32_t)w * (uint32_t)h);
    RESET_X_RANGE();
}

/**
 * Fill entire screen with a color.
 * 
 * No masking or overdraw checking is performed. FRAME_ID_BIT is not set.
 * 
 * Calls the `flood` subroutine, which will automatically optimize color filling
 * if the low and high bytes of the color data are identical.
 *
 * @param color 16-bit 565 RGB color code
 */
void Arduino_TFTLCD::fillScreen(uint16_t color) {
    ZERO_XY();
    flood(color, (long)TFTWIDTH * (long)TFTHEIGHT);
}

/**
 * Writes a single pixel to the diplay.
 *
 * Supports masked erasing. If `do_masking` is True and all foreground pixels
 * from the current and pervious frame were drawn with their corresponding
 * FRAME_ID_BITs set, then it will not erase pixels drawn in the current frame.
 *
 * @param y (uint16_t): the vertical position of the pixel
 * @param permuted_color (uint16_t): 16-bit color, with bits *already permuted*
 *      to match Arduino's output pins as hooked up to the TFT display shield. 
 *      Use macro `BIT_TO_PORT_PERMUTATION_16(color)` to shuffle color bits 
 *      before calling this function.
 */
void Arduino_TFTLCD::colorPixel(uint16_t y, uint16_t permuted_color) {
    #ifndef DISABLE_MASKING_AND_OVERDRAW
        // The FRAME_ID_BIT for the current color  is flipped on alternate lines
        // to reduce visible distortions of the color value
        uint8_t permuted_line_flag = PERMUTED_FRAME_ID_FLAG8*(y&1);
        // Re-arrange bits of current mask_flag to better match Arduino's output
        // ports as they are attached to the TFT display shield. This is the 
        // current FRAME_ID_BIT. x-or it with the current line flag so that it 
        // flips between 0 and 1 on alternate lines, to reduce visible disruptions.
        uint8_t permuted_mask_test = BIT_TO_PORT_PERMUTATION(mask_flag)^permuted_line_flag;
        if (do_masking) {
            // If masking is active, we need to check the current value of the
            // pixel before writing it (slow! but necessary for some graphics
            // routines).
            START_READING();
            DELAY1
            // We only need to read the first byte of the color, since this is
            // where the FRAME_ID_BIT is stored. 
            uint8_t R = PERMUTED_QUICK_READ;
            STOP_READING();
            // If the FRAME_ID_BIT is set, (i.e. matches permuted_mask_test), then
            // do not color it. This is mainly used to avoid erasing pixels that
            // that have just been drawn in the current frame. 
            if ((R&PERMUTED_FRAME_ID_FLAG8)==permuted_mask_test) return;
        } else {
            // If masking is turned OFF, we don't need to check FRAME_ID_BIT. 
            // But, we should (probably) at least set it, so that other drawing
            // routines can tell whether we've colored the pixel in this frame yet.
            // (Comment out these lines if you are not using FRAME_ID_BIT, and
            // would like to avoid having FRAME_ID_BIT subtly mess up the colors)
            permuted_color &= PERMUTED_FRAME_ID_MASK16;
            permuted_color |= (uint16_t)(permuted_mask_test)<< 8;
        }
    #endif 
    // Write pixel data
    // If masking is ON, and the current pixel does not have the FRAME_ID_FLAG
    // set, then we write the color data (typically this is a background color
    // being used to erase stale pixels)
    START_PIXEL_DATA();
    SEND_PERMUTED_PIXEL(permuted_color);
}

/**
 * Write a single 16-bit pixel to the display. 
 *
 * No masking or overdraw checking is performed. FRAME_ID_BIT is not set.
 *
 * @param x Horizontal location of the pixel
 * @param y Vertical location of the pixel
 * @param color 16-bit RRRRGGGGGGBBBBB color data (not permuted)
 */
void Arduino_TFTLCD::drawPixel(int16_t x, int16_t y, uint16_t color) {
    #ifdef DO_CLIP
        // Clipping draw commands to ensure they lie within the display area
        // takes time. If the user is willing to guarantee that all drawing
        // commands are in-bounds, clipping can be disabled by unsetting the
        // compile-time flag DO_CLIP
        if((x<0)||(y<0)||(x>=_width)||(y>=_height)) return;
    #endif
    SET_XY_LOCATION(x,y);
    // Bits are permuted to match port to pin configuration on TFT shield.
    // (This is device dependent)
    BIT_TO_PORT_PERMUTATION_16(color);
    colorPixel(y,color);
}

/**
 * Fill a vertical line on the display. 
 *
 * No masking or overdraw checking is performed. FRAME_ID_BIT is not set.
 *
 * @param x Horizontal location of the pixel
 * @param y Vertical location of the pixel
 * @param color 16-bit RRRRGGGGGGBBBBB color data (not permuted)
 */
void Arduino_TFTLCD::drawFastVLine(int16_t x, int16_t y, int16_t length, uint16_t color)
{
    #ifdef DO_CLIP
        // Clipping draw commands to ensure they lie within the display area
        // takes time. If the user is willing to guarantee that all drawing
        // commands are in-bounds, clipping can be disabled by unsetting the
        // compile-time flag DO_CLIP
        int16_t y2=y+length-1;
        if(length<=0||x<0||x>=_width||y>=_height||y2<0) return;
        if(y<0) {length+=y;y=0;}
        if(y2>=_height) {y2=_height-1;length=y2-y+1;}
    #endif
    SET_XY_RANGE(x,x,y);
    flood(color, length);
    RESET_X_RANGE();
}


/**
 * Fast horizontal line routine optimized for shading in triangles for 3D 
 * rendering.
 * 
 * When drawing triangles, large contiguous areas will be masked out. So
 * instead we store a list of offsets and lengths that are /not/ masked out,
 * and just draw those To do this, we start reading the color data. If it is
 * masked, we continue until it is not masked, and mark that position. We keep
 * reading unmasked data until we come to a masked pixel, or are at the end of
 * the line. We then draw the pixel data. We use the continue read data to pick
 * up where we left off.
 *
 * @param x horizontal position of start of horizontal line
 * @param y vertical position of start of horizontal line
 * @param length width of horizontal line
 * @param color 16-bit color value for this line
 */
void Arduino_TFTLCD::drawFastHLine(int16_t x, int16_t y, int16_t length, uint16_t color){
    if (length<1) return;
    #ifdef DO_CLIP
        // Clipping draw commands to ensure they lie within the display area
        // takes time. If the user is willing to guarantee that all drawing
        // commands are in-bounds, clipping can be disabled by unsetting the
        // compile-time flag DO_CLIP
        int16_t x2 = x+length-1;
        if(length<=0||y<0||y>=_height||x>=_width||x2<0) return;
        if(x<0) {length+=x; x=0;}
        if(x2>=_width) {x2=_width-1; length=x2-x+1;}
    #endif
    #ifndef DISABLE_MASKING_AND_OVERDRAW
        // Set up the FRAME_ID_BIT.
        // This bit is flipped on every-other line (`line_flag`). 
        uint8_t line_flag = FRAME_ID_FLAG8*(y&1);
        uint8_t permuted_mask_test = BIT_TO_PORT_PERMUTATION(mask_flag^line_flag);
        uint8_t permuted_background_mask = BIT_TO_PORT_PERMUTATION( (background_color>>8) & QUICK_COLOR_MASK );
        if (!do_masking) {
            // If masking is OFF, then we set the FRAME_ID_BIT in the color data
            color &= FRAME_ID_MASK16;
            color |= (uint16_t)(mask_flag^line_flag)<<8;
        }
    #endif
    // Retain only the high byte of the color data. We will use the `fastFlood`
    // routine, so the high and low bytes of the color data will be the same.
    // (User should restrict to subset of "fast" colors where the high and low
    // bytes match to avoid excessively weird colors)
    color >>= 8;
    SET_Y_LOCATION(y);
    #ifndef DISABLE_MASKING_AND_OVERDRAW
        // Overdrawing and masking are similar.
        // *Masking:* used to erase (cover with background color). It avoids 
        // filling pixels if the FRAME_ID_BIT indicates that said pixel was colored
        // as part of the current frame.
        // *Overdraw:* used in 3D rendering to handle occlusions: new triangles 
        // are not drawn if another triangle has already been drawn there in the
        // current frame.
        if (do_masking || do_overdraw) {
            uint8_t in_segment = 0;
            uint8_t start      = x;
            uint8_t stop       = x+length;
            uint8_t i          = x;
            while (i<stop) {
                SET_X_LOCATION(i);
                START_READING();
                while (i<stop) {
                    // Check if color is *NOT* background and has FRAME_ID_BIT set
                    // We only need to read the first (high) byte of color data.
                    uint8_t read = PERMUTED_QUICK_READ;
                    uint8_t is_masked = 
                       (read & PERMUTED_QUICK_COLOR_MASK) != permuted_background_mask
                    && (read & PERMUTED_FRAME_ID_FLAG8  ) == permuted_mask_test;
                    // Skip the second (low) byte of color data
                    SEND_DATA;
                    READY_READ;
                    SEND_DATA; 
                    if (is_masked) {
                        if (in_segment) {
                            // If current pixel is masked, but previous pixels
                            // weren't, stop to fill in those previous pixels. 
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
                        // If previous pixels were masked, but this one isn't, 
                        // start keeping track of the current segment. We will fill
                        // it in later, when we encounter a new masked region or
                        // reach the end of the horizontal line.
                        start = i;
                        in_segment = 1;
                    }     
                    READY_READ;
                    i++;
                }
            }
            STOP_READING();
            if (in_segment) {
                SET_X_LOCATION(start);
                fastFlood(color,i-start);
            }
        } else {
            // If neither masking nor overdraw is on, simply fill the line segment.
            SET_X_LOCATION(x);
            fastFlood(color,length);
        }
    #else 
        // If masking and overdraw disabled, simply fill the line segment.
        SET_X_LOCATION(x);
        fastFlood(color,length);
    #endif
}

/**
 * Bresenham's line algorithm
 * 
 * @param x0 horizontal starting coordinate
 * @param y0 vertical starting coordinate
 * @param x1 horizontal end coordinate
 * @param y1 vertical end coordinate
 * @param color 16-bit 565 RGB fill color
 */
void Arduino_TFTLCD::drawLine(int16_t x0, int16_t y0, 
                              int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep)   { swap(x0, y0); swap(x1, y1); }
    if (x0 > x1) { swap(x0, x1); swap(y0, y1); }
    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep;
    BIT_TO_PORT_PERMUTATION_16(color);
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
void     Arduino_TFTLCD::overdraw_on()  {do_overdraw = 1;}
void     Arduino_TFTLCD::overdraw_off() {do_overdraw = 0;}
void     Arduino_TFTLCD::masking_on()   {do_masking  = 1;}
void     Arduino_TFTLCD::masking_off()  {do_masking  = 0;}
void     Arduino_TFTLCD::flip_mask()    {mask_flag  ^= FRAME_ID_FLAG8;}

////////////////////////////////////////////////////////////////////////////
// LOW-LEVEL DATA IO ROUTINES
////////////////////////////////////////////////////////////////////////////

/**
 * **Not implemented.** The masked and overdraw rendering routines read pixel
 * data in an optimized way as needed. 
 * 
 * @param x horizontal coordinate of pixel to read
 * @param y vertical coordinate of pixel to read
 */
uint16_t Arduino_TFTLCD::readPixel(int16_t x, int16_t y) {
    return 0;
}


////////////////////////////////////////////////////////////////////////////
// Fast drawing extensions.
// Support only a limited color pallet
////////////////////////////////////////////////////////////////////////////

/** 
 * X-ORs Pixel data with mask
 *
 * No masking or overdraw (occlusion) checking is performed.
 *
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 * 
 * @param mask
 * @param length
 */
void Arduino_TFTLCD::fastXORFlood(uint8_t mask, uint8_t length) {
    // avr-gcc supports variable length arrays on the stack
    uint8_t colors[length];
    mask = BIT_TO_PORT_PERMUTATION(mask);
    // First read the pixels
    START_READING();       
    for(uint16_t i=0; i<length; i++) {
        uint8_t read = PERMUTED_QUICK_READ;
        SEND_DATA;
        READY_READ;
        SEND_DATA;
        colors[i] = read^mask;
        READY_READ;
    }
    // Then write the x-ored pixels back
    STOP_READING();
    START_PIXEL_DATA();
    for(uint16_t i=0; i<length; i++) {
        WRITE_PERMUTED_BUS(colors[i]);
        CLOCK_1;
    }
}

/**
 * Fastest way to fill the entire screen with a color (used to erase screen). 
 *
 * No masking or overdraw (occlusion) checking is performed.
 *
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 * 
 * @param color A *fast* color (low and high bytes are the same).
 */
void Arduino_TFTLCD::fastFillScreen(uint8_t color) {
#ifdef SAVE_SPACE
    fillScreen(color*0x0101);
#else
    ZERO_XY();
    START_PIXEL_DATA();
    WRITE_BUS_FAST(color);
    for (uint16_t i=0; i<300; i++) CLOCK_256; 
#endif
}

/**
 * Fastest way to color a single pixel
 *
 * No masking or overdraw (occlusion) checking is performed.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors. 
 * 
 * @param x horizontal location of pixel
 * @param y vertical location of pixel
 * @param color A *fast* color (low and high bytes are the same).
 */
void Arduino_TFTLCD::fastPixel(uint8_t x, uint16_t y, uint8_t color) {
#ifdef SAVE_SPACE
    drawPixel(x,y,color*0x0101);
#else
    SET_XY_LOCATION(x,y);
    START_PIXEL_DATA();
    WRITE_BUS(color);
    CLOCK_1;
#endif
}

/**
 * Fast X-OR fill of a rectangular region.
 *
 * No masking or overdraw (occlusion) checking is performed.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors. 
 * 
 * @param x horizontal coordinate of start of rectangular regi on
 * @param y vertical coordinate of start of rectangular region
 * @param w width of rectangular region (assumes dimension <=240)
 * @param h height of rectangular region
 * @param mask color bit mask to apply
 */
void Arduino_TFTLCD::fastXORRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t mask) {
    uint8_t  x2=x+w-1;
    SET_X_RANGE(x,x2);
    for (int i=0; i<h; i++) {
        SET_Y_LOCATION(i+y);
        fastXORFlood(mask,w);
    }
    RESET_X_RANGE();
}

/**
 * Fast fill of a rectangular region
 *
 * No masking or overdraw (occlusion) checking is performed.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 *
 * @param x horizontal coordinate of start of rectangular regi on
 * @param y vertical coordinate of start of rectangular region
 * @param w width of rectangular region (assumes dimension <=240)
 * @param h height of rectangular region
 * @param c A *fast* color (low and high bytes are the same).
 */
void Arduino_TFTLCD::fastFillRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c) {
#ifdef SAVE_SPACE
    fillRect(x,y,w,h,c*0x0101);
#else
    uint8_t  x2=x+w-1;
    SET_XY_RANGE(x,x2,y);
    fastFlood(c,w*h);
    RESET_X_RANGE();
#endif
}

/**
 * Fast draw of rectangular outline.
 *
 * No masking or overdraw (occlusion) checking is performed.
 *
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 *
 * @param x horizontal coordinate of start of rectangular region
 * @param y vertical coordinate of start of rectangular region
 * @param w width of rectangular region (assumes dimension <=240)
 * @param h height of rectangular region
 * @param c A *fast* color (low and high bytes are the same).
 */
void Arduino_TFTLCD::fastDrawRect(uint8_t x, uint16_t y, uint8_t w, uint16_t h, uint8_t c) {
    // TODO: should we set FRAME_ID_BIT here as in fastestHLine?
#ifdef SAVE_SPACE
    drawRect(x,y,w,h,c*0x0101);
#else
    fastestHLine(x, y, w, c);
    fastestHLine(x, y+h-1, w, c);
    fastestVLine(x, y, h, c);
    fastestVLine(x+w-1, y, h, c);
#endif
}

/**
 * Fast draw of a vetical line. 
 * 
 * No masking or overdraw (occlusion) checking is performed.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 *
 * @param x horizontal coordinate of start of line
 * @param y vertical coordinate of start of line
 * @param h height of line
 * @param color A *fast* color (low and high bytes are the same)
 */
void Arduino_TFTLCD::fastestVLine(uint8_t x, uint16_t y, uint16_t h, uint8_t color) {
    // TODO: should we set FRAME_ID_BIT here as in fastestHLine?
    SET_XY_RANGE(x,x,y);
    fastFlood(color,h);
    RESET_X_RANGE();
}

/**
 * Fastest way to draw a horizontal line.
 *
 * No masking or overdraw (occlusion) checking is performed.
 *
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 *
 * @param x horizontal coordinate of start of line
 * @param y vertical coordinate of start of line
 * @param w width of line
 * @param color A *fast* color (low and high bytes are the same)
 */
void Arduino_TFTLCD::fastestHLine(uint8_t x, uint16_t y, uint16_t w, uint8_t color) {
    #ifndef DISABLE_MASKING_AND_OVERDRAW
        if (!do_masking) {
            // (`do_masking` is true if we are currently doing masked erasing.)
            // Masked erasing fills in pixels with a background color, skipping
            // pixels for which the FRAME_ID_BIT matches the current frame.
            // Outside of masked erase mode, we should set the FRAME_ID_BIT 
            // correctly to mark pixels from the current frame.
            // TODO: why doesn't the FRAME_ID_BIT setting code appear in the other
            // fast drawing routines?
            uint8_t line_flag = FRAME_ID_FLAG8*(y&1);
            color &= FRAME_ID_MASK8;
            color |= mask_flag^line_flag;
        }
    #endif
    SET_XY_LOCATION(x,y);
    fastFlood(color,w);
}

/**
 * Fastest way to draw the outline of a triangle.
 * 
 * No masking or overdraw (occlusion) checking is performed.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 * 
 * @param x0 horizontal coordinate of first point
 * @param y0 vertical coordinate of first point
 * @param x1 horizontal coordinate of second point
 * @param y1 vertical coordinate of second point
 * @param x2 horizontal coordinate of third point
 * @param y2 vertical coordinate of third point
 * @param color A *fast* color (low and high bytes are the same)
 */
void Arduino_TFTLCD::fastDrawTriangle(
    uint8_t x0, uint16_t y0, 
    uint8_t x1, uint16_t y1, 
    uint8_t x2, uint16_t y2, uint8_t color) {
  fastLine(x0, y0, x1, y1, color);
  fastLine(x1, y1, x2, y2, color);
  fastLine(x2, y2, x0, y0, color);
}

/**
 * Fastest way to fill a triangular region
 * 
 * No masking or overdraw (occlusion) checking is performed.
 *
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 *
 * @param x0 horizontal coordinate of first point
 * @param y0 vertical coordinate of first point
 * @param x1 horizontal coordinate of second point
 * @param y1 vertical coordinate of second point
 * @param x2 horizontal coordinate of third point
 * @param y2 vertical coordinate of third point
 * @param color A *fast* color (low and high bytes are the same)
 */
void Arduino_TFTLCD::fastFillTriangle(
    uint8_t _x0, uint16_t _y0, 
    uint8_t _x1, uint16_t _y1, 
    uint8_t _x2, uint16_t _y2, uint8_t color) {
    // There's sort of a numerical problem I'm still pinning down with
    // using the short unsigned types. Just convert them for now
    int x0 = _x0;
    int x1 = _x1;
    int x2 = _x2;
    int y0 = _y0;
    int y1 = _y1;
    int y2 = _y2;
    #ifdef SAVE_SPACE
        fillTriangle(x0,y0,x1,y1,x2,y2,color*0x0101);
    #else
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
    #endif
}

/**
 * Fastest way to draw a line. Uses Bresenham's line algorithm.
 *
 * This routine correctly sets the FRAME_ID_BIT per scanline, but does not 
 * support masked erasing or overdraw checking.
 * 
 * This routine assumes that it is reading and writing *fast* colors, for which
 * the low and high bytes are the same. It will produce undefined results if
 * used on regular 565 RGB 16-bit colors.
 * 
 * @param x0 horizontal starting coordinate
 * @param y0 vertical starting coordinate
 * @param x1 horizontal end coordinate
 * @param y1 vertical end coordinate
 * @param color 16-bit 565 RGB fill color
 */
void Arduino_TFTLCD::fastLine(
    uint8_t x0, uint16_t y0, 
    uint8_t x1, uint16_t y1, 
    uint8_t color) {
    uint16_t dx = x1>x0?x1-x0:x0-x1;
    uint16_t dy = y1>y0?y1-y0:y0-y1;
    
    #ifndef DISABLE_MASKING_AND_OVERDRAW
        // Clear the FRAME_ID_BIT in the color data
        color &= FRAME_ID_MASK8;
    #endif
    
    if (dy>dx) {
        // Line is steep: vertical span exceeds horizontal span
        // May also be a purely vertical line
        // Ensure that first point of line has smaller y coordinate
        if (y0>y1) {swapU16(y0,y1); swapU8(x0,x1);}
        int16_t err   = dy/2;
        int16_t xstep = x0<x1?1:-1;
        // Draw line as sequence of vertical lines. 
        #ifndef DISABLE_MASKING_AND_OVERDRAW
            // Need to toggle the FRAME_ID_BIT on alternate scanlines. 
            // This means that the color data changes on alternate lines.
            // Set FRAME_ID_BIT to appropriate value for y0
            color |= mask_flag^(FRAME_ID_FLAG8*(y0&1));
        #endif
        SET_XY_RANGE(x0,x0,y0);
        START_PIXEL_DATA();
        while (y0<=y1) {
            // Write the current color to the display
            WRITE_BUS(color);
            CLOCK_1;
            #ifndef DISABLE_MASKING_AND_OVERDRAW
                // Toggle FRAME_ID_BIT when advancing vertical position
                color ^= FRAME_ID_FLAG8;
            #endif
            y0++;
            err-=dx;
            if (err<0) {
              // Advance/decrease horizontal position along the line.
              x0 += xstep;
              SET_XY_RANGE(x0,x0,y0);
              START_PIXEL_DATA();
              // TODO: is it a problem that we might have flipped the draw order
              // *after* initializing dx and dy? Is this a bug?
              err+= dy;
            }
        }
        RESET_X_RANGE();
    } else {
        // Line is not steep: horizontal span exceeds vertical span
        // May also be a purely horizontal line
        // Ensure that first point of line has smaller x coordinate
        if (x0>x1) {swapU16(y0,y1); swapU8(x0,x1);}
        int16_t err = dx/2;
        int16_t ystep = y0<y1?1:-1;
        // Draw line as sequence of horizontal lines
        #ifndef DISABLE_MASKING_AND_OVERDRAW
            // Need to toggle the FRAME_ID_BIT on alternate scanlines. 
            // This means that the color data changes on alternate lines.
            // Set FRAME_ID_BIT to appropriate value for y0
            color |= mask_flag^(FRAME_ID_FLAG8*(y0&1));
        #endif
        SET_XY_LOCATION(x0,y0);
        START_PIXEL_DATA();
        WRITE_BUS(color);
        while (x0<=x1) {
            CLOCK_1;
            err -= dy;
            x0++;
            if (err < 0) {
                y0 += ystep;
                SET_XY_LOCATION(x0,y0);
                START_PIXEL_DATA();
                #ifndef DISABLE_MASKING_AND_OVERDRAW
                    // Toggle FRAME_ID_BIT when advancing vertical position
                    color ^= FRAME_ID_FLAG8;
                #endif
                WRITE_BUS(color);
                err += dx;
            }
        }
    }
}  



