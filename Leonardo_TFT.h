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

         1  0
B  7  6  5  4  3  2  1  0

      5
C  7  6  5  4  3  2  1  0

   6        4        2  3
D  7  6  5  4  3  2  1  0

      7
E  7  6  5  4  3  2  1  0

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


      1 0 - - - -
B 7 6 5 4 3 2 1 0
    5 -
C 7 6 5 4 3 2 1 0
  6 x x 4 x x 2 3
D 7 6 5 4 3 2 1 0  
    7
E 7 6 5 4 3 2 1 0

Port D is a tricky customer.
4 is in the right spot. 
6 needs to be moved right.
2 needs to be moved left.
3 needs to be moved left 3 places.


6xx4xx23
-6xx4xx2

66444222
76543210
01010100

76543210  
6xx4xx23


   10
B76543210
PORTB =
(((d>>0)&1)<<4)|
(((d>>1)&1)<<5);

  5
C76543210
PORTC = 
((d>>5)&1)<<6;

 6  4  23
D76543210
PORTD = 
(((d>>2)&1)<<1)|
(((d>>3)&1)<<0)|
(((d>>4)&1)<<4)|
(((d>>6)&1)<<7);

  7
E76543210
PORTE = 
((d>>7)&1)<<6;

*/
#define FRAME_ID_BIT 3
#define QUICK_READ_MASK   0b11011100


#define BMASK 0b00110000
#define CMASK 0b01000000
#define DMASK 0b10010011
#define EMASK 0b01000000

/*
//0
#define  PB4 ((PINB>>4)&1)
#define  PB5 ((PINB>>5)&1)
#define  PD1 ((PIND>>1)&1)
#define  PD0 ((PIND>>0)&1)
#define  PD4 ((PIND>>4)&1)
#define  PC6 ((PINC>>6)&1)
#define  PD7 ((PIND>>7)&1)
#define  PE6 ((PINE>>6)&1)
//7
#define READ_BYTE (\
    (PB4<<0)|\
    (PB5<<1)|\
    (PD1<<2)|\
    (PD0<<3)|\
    (PD4<<4)|\
    (PC6<<5)|\
    (PD7<<6)|\
    (PE6<<7))
*/


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

#define WRITE_BUS_FAST(b) {\
    WRITE_BUS(b);\
}

// This needs to grab at least the mask aka frame_ID bit and should
// also grab whatever bits we want to use to recognize background vs.
// foreground colors. 
#define QUICK_READ READ_BYTE


