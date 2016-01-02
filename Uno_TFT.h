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
PC4: RESET LINE keep high
PC3: CS line keep low while driving, leave high to disable interface
PC2: Data send bit. 1 for sending data, 0 for sending command
PC1: Write clock bit. Triggered on rising edge, I think. 
PC0: Chip select line. 1 for enabel. Left high by driver code. 

#define READY_COMMAND     PORTC=0b1110001
#define SEND_COMMAND      PORTC=0b1110011
#define READY_DATA        PORTC=0b1110101
#define SEND_DATA         PORTC=0b1110111
#define READY_READ        PORTC=0b1110110
#define REQUEST_READ      PORTC=0b1110010
*/

#define RS_PIN 4
#define CS_PIN 3
#define CD_PIN 2
#define WR_PIN 1
#define RD_PIN 0
#define CONTROLPORT PORTC

#define FRAME_ID_BIT 3
#define QUICK_READ_MASK   0b11111100

#define READ_BYTE ((PIND&B11111100)|(PINB&B00000011))

//////////////////////////////////////////////////////////////////////////
// Formerly pin_magic.h

#define WRITE_BUS(b) {\
    PORTB=PORTD=(b);\
}

#define WRITE_BUS_FAST(b) {\
    PORTD=(b);\
}

#define setWriteDir() {\
    DDRD |=  B11111100;\
    DDRB |=  B00000011;\
}

#define setReadDir()  {\
    DDRD &= ~B11111100;\
    DDRB &= ~B00000011;\
}

//////////////////////////////////////////////////////////////////////////
// Color reading and writing

#define QUICK_READ PIND



