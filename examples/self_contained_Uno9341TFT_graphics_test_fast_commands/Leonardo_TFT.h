/*
Leonardo is different from Uno. The control lines are now all on PORTF
LCD_RST PF1
LCD_CS  PF4
LCD_CD  PF5
LCD_WR  PF6
LCD_RD  PF7
*/
#define RS_PIN 1
#define CS_PIN 4
#define CD_PIN 5
#define WR_PIN 6
#define RD_PIN 7
#define CONTROLPORT PORTF


/*
The data lines are not set up as nicely for fast IO, as the ports are
not mapped contiguously to the data lines. Here is a map of the pins,
as you can see it's a bit of a mess.

LCD_00  PB4
LCD_01  PB5
LCD_02  PD1
LCD_03  PD0
LCD_04  PD4
LCD_05  PC6
LCD_06  PD7
LCD_07  PE6

R  R  R  R  R  G  G  G
G  G  G  B  B  B  B  B
E  D  C  D  D  D  B  B
6  7  6  4  0  1  5  4 

The following pins are critical for basic 3-bit color resolution: E6 D4 D1
We will always need to reat at least E and D then. We can still use the 
low order red bit. 

      1 0
B 7 6 5 4 3 2 1 0
    5
C 7 6 5 4 3 2 1 0
  6     4     2 3
D 7 6 5 4 3 2 1 0  
    7
E 7 6 5 4 3 2 1 0

Propose the following permutation of color data for faster IO
Note that bits 7,2 and 4 are the high-order color bits and should be 
Preserved in even fast-reads. 1,3, and 6 are also fairly important.
For fast I/O suggest the following: 
  6 7 x 4 x x 2 3
Optionally, it is perhaps OK to add PORTB
  6 7 x 4 1 0 2 3
Full permutation -- send color bits to these positions
  6 7 5 4 1 0 2 3
Need macros to inteconvert bit permutations
    76543210
    67541023
*/

#define FRAME_ID_BIT 3
#define QUICK_READ_MASK   0b11011100

#define BMASK 0b00110000
#define CMASK 0b01000000
#define DMASK 0b10010011
#define EMASK 0b01000000

#define READ_BYTE (\
     (((PINE & B01000000) | (PIND & B00000010)) << 1) |       \
     (((PINC & B01000000) | (PIND & B10000000)) >> 1) |       \
      ((PIND & B00000001) << 3) | ((PINB & B00110000) >> 4) | \
       (PIND & B00010000))

#define WRITE_BUS(d) {                                                   \
    uint8_t dr1 = (d) >> 1, dl1 = (d) << 1;                                   \
    PORTE = (PORTE & B10111111) | (dr1 & B01000000);                          \
    PORTD = (PORTD & B01101100) | (dl1 & B10000000) | (((d) & B00001000)>>3) |\
                                  (dr1 & B00000010) |  ((d) & B00010000);     \
    PORTC = (PORTC & B10111111) | (dl1 & B01000000);                          \
    PORTB = (PORTB & B11001111) |(((d) & B00000011)<<4);                      \
}

#define read8inline(result) {\
    RD_ACTIVE;\
    DELAY7;\
    result = READ_BYTE;\
    RD_IDLE;\
}


#define setWriteDir() {               \
DDRE |=  B01000000; DDRD |=  B10010011;   \
DDRC |=  B01000000; DDRB |=  B00110000; }

#define setReadDir() {                \
DDRE &= ~B01000000; DDRD &= ~B10010011;   \
DDRC &= ~B01000000; DDRB &= ~B00110000; }

#define WRITE_BUS_FAST(b) {\                                                  \
    uint8_t dr1 = (d) >> 1, dl1 = (d) << 1;                                   \
    PORTE = (PORTE & B10111111) | (dr1 & B01000000);                          \
    PORTD = (PORTD & B01101100) | (dl1 & B10000000) | (((d) & B00001000)>>3) |\
                                  (dr1 & B00000010) |  ((d) & B00010000);     \
}

//////////////////////////////////////////////////////////////////////////
// Permutations between color bit order and port bit order.
// In the Leonardo these are complex, and are computed ahead of time
// to speed up IO.
// On the Uno, these permutations can be replaced by the identity map.

// Send color data from 565 format into the permuted representation
// for faster IO
/*
#define BIT_TO_PORT_PERMUTATION(b) (\
    ((((b)>>0)&1)<<2)|\
    ((((b)>>1)&1)<<3)|\
    ((((b)>>2)&1)<<1)|\
    ((((b)>>3)&1)<<0)|\
    ((((b)>>4)&1)<<4)|\
    ((((b)>>5)&1)<<5)|\
    ((((b)>>6)&1)<<7)|\
    ((((b)>>7)&1)<<6)\
)
*/

#define BIT_TO_PORT_PERMUTATION(b) (\
    (((b)&0b00000011)<<2)|\
    (((b)&0b01000000)<<1)|\
    (((b)&0b00110000))|\
    (((b)&0b10000100)>>1)|\
    (((b)&0b00001000)>>3)\
)


// Take color data form port permutation back to the color 565 bit order
/*
#define PORT_TO_BIT_PERMUTATION(b) (\
    ((((b)>>2)&1)<<0)|\
    ((((b)>>3)&1)<<1)|\
    ((((b)>>1)&1)<<2)|\
    ((((b)>>0)&1)<<3)|\
    ((((b)>>4)&1)<<4)|\
    ((((b)>>5)&1)<<5)|\
    ((((b)>>7)&1)<<6)|\
    ((((b)>>6)&1)<<7)\
)
*/

#define PORT_TO_BIT_PERMUTATION(b) (\
    (((b)&0b00001100)>>2)|\
    (((b)&0b01000010)<<1)|\
    (((b)&0b00000001)<<3)|\
    (((b)&0b00110000))|\
    (((b)&0b10000000)>>1)\
)


#define WRITE_PERMUTED_BUS(d) {\
    PORTE = (d) & B01000000;\
    PORTD = (d) & B10010011;\
    PORTB = ((d)<<2) & B00110000;\
    PORTC = ((d)<<1) & B01000000;\
}

// This is a sanity check
#define IDENTITY(b) BIT_TO_PORT_PERMUTATION(PORT_TO_BIT_PERMUTATION(b))

#define PERMUTED_QUICK_READ (\
    (PIND & 0b10010011)\
  | (PINE & 0b01000000)\
  |((PINB & 0b00110000)>>2)\
)

// This needs to grab at least the mask aka frame_ID bit and should
// also grab whatever bits we want to use to recognize background vs.
// foreground colors. Frame bit is on D
#define QUICK_READ (\
    ((PINE&B01000000|PIND&B00000010) << 1) |       \
    ((PIND&B10000000) >> 1) |       \
    ((PIND&B00000001) << 3) | \
     (PIND&B00010000)\
)



