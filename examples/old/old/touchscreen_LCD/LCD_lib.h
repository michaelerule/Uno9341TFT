#include <Arduino.h>

// The breakout board exposes the following pins
// LED: powers the backlight
// SCK: serial clock. SDA pin is sampled on the rising edge.
// SDA: serial data line.
// A0: connected to a pin called D/XC on the driver chip
//     pull this pin low to send a command over serial 
//     pull this pin high to send data over serial
// RESET: should be pulled high. send a low pulse at least 10ns to reset
// CS: this pin needs to be pulled low to enable serial communication
// GND: ground
// VCC: power

// Command definitions. 
// The datasheet defines a collection of 8-bit commands.
// Not all commands are documented, apparently. 
#define START_FRAME     0x2C
#define EXIT_SLEEP_MODE 0x11
#define DISPLAY_ON      0x29
#define DISPLAY_OFF     0x28

// register names

#define TFTLCD_DRIV_ID_READ       0x00
#define TFTLCD_DRIV_OUT_CTRL      0x01
#define TFTLCD_DRIV_WAV_CTRL      0x02
#define TFTLCD_ENTRY_MOD          0x03
#define TFTLCD_RESIZE_CTRL        0x04
#define TFTLCD_DISP_CTRL1         0x07
#define TFTLCD_DISP_CTRL2         0x08
#define TFTLCD_DISP_CTRL3         0x09
#define TFTLCD_DISP_CTRL4         0x0A
#define TFTLCD_FRM_MARKER_POS     0x0D
#define TFTLCD_POW_CTRL1          0x10
#define TFTLCD_POW_CTRL2          0x11
#define TFTLCD_POW_CTRL3          0x12
#define TFTLCD_POW_CTRL4          0x13
#define TFTLCD_GRAM_HOR_AD        0x20
#define TFTLCD_GRAM_VER_AD        0x21
#define TFTLCD_RW_GRAM            0x22
#define TFTLCD_VCOMH_CTRL         0x29
#define TFTLCD_FRM_RATE_COL_CTRL  0x2B
#define TFTLCD_GAMMA_CTRL1        0x30
#define TFTLCD_GAMMA_CTRL2        0x31
#define TFTLCD_GAMMA_CTRL3        0x32
#define TFTLCD_GAMMA_CTRL4        0x35
#define TFTLCD_GAMMA_CTRL5        0x36
#define TFTLCD_GAMMA_CTRL6        0x37
#define TFTLCD_GAMMA_CTRL7        0x38
#define TFTLCD_GAMMA_CTRL8        0x39
#define TFTLCD_GAMMA_CTRL9        0x3C
#define TFTLCD_GAMMA_CTRL10       0x3D
#define TFTLCD_HOR_START_AD       0x50
#define TFTLCD_HOR_END_AD         0x51
#define TFTLCD_VER_START_AD       0x52
#define TFTLCD_VER_END_AD         0x53
#define TFTLCD_GATE_SCAN_CTRL1    0x60
#define TFTLCD_GATE_SCAN_CTRL2    0x61
#define TFTLCD_PART_IMG1_DISP_POS 0x80
#define TFTLCD_PART_IMG1_START_AD 0x81
#define TFTLCD_PART_IMG1_END_AD   0x82
#define TFTLCD_PART_IMG2_DISP_POS 0x83
#define TFTLCD_PART_IMG2_START_AD 0x84
#define TFTLCD_PART_IMG2_END_AD   0x85
#define TFTLCD_PANEL_IF_CTRL1     0x90
#define TFTLCD_PANEL_IF_CTRL2     0x92

/*
void write_byte(uint8_t data) {
    digitalWrite(LCD_WR,LOW);
    PORTD = data;
    PORTB = data;
    digitalWrite(LCD_WR,HIGH);
}
*/

inline void write_command(uint16_t command)
{
    PORTC=0b1110001;
    PORTB=PORTD=command>>8;
    PORTC=0b1110011;
    PORTC=0b1110001;
    PORTB=PORTD=command;
    PORTC=0b1110011;
    PORTC=0b1111011;
}

inline void write_data(uint16_t data)
{
    PORTC=0b1110101;
    PORTB=PORTD=data>>8;
    PORTC=0b1110111;
    PORTC=0b1110101;
    PORTB=PORTD=data;
    PORTC=0b1110111;
}


inline void set_register(uint16_t addr, uint16_t data) {
   write_command(addr);
   write_data(data);
}

void start_frame(void) {
  set_register(TFTLCD_GRAM_HOR_AD, 0);   // GRAM Address Set (Horizontal Address) (R20h)
  set_register(TFTLCD_GRAM_VER_AD, 0);   // GRAM Address Set (Vertical Address) (R21h)
  write_command(TFTLCD_RW_GRAM);         // Write Data to GRAM (R22h)
}

void reset()
{
    digitalWrite(LCD_RST,LOW);
    delay(50);
    digitalWrite(LCD_RST,HIGH);
    delay(50);
}

void initialize_LCD()
{
  /* I apologize this initialization is modified from some undocumented
  code from the vendor. It even sends commands that aren't documented
  in the datasheet. For the most part we have no idea what it does */
  pinMode(LCD_RST,OUTPUT);
  pinMode(LCD_CS ,OUTPUT);
  pinMode(LCD_RS ,OUTPUT);
  pinMode(LCD_WR ,OUTPUT);
  pinMode(LCD_RD ,OUTPUT);
  pinMode(LCD_D0 ,OUTPUT);
  pinMode(LCD_D1 ,OUTPUT);
  pinMode(LCD_D2 ,OUTPUT);
  pinMode(LCD_D3 ,OUTPUT);
  pinMode(LCD_D4 ,OUTPUT);
  pinMode(LCD_D5 ,OUTPUT);
  pinMode(LCD_D6 ,OUTPUT);
  pinMode(LCD_D7 ,OUTPUT);
  reset();
  set_register(TFTLCD_DRIV_OUT_CTRL, 0x0100);
  set_register(TFTLCD_DRIV_WAV_CTRL, 0x0700);
  set_register(TFTLCD_ENTRY_MOD,  0x1030);   
  set_register(TFTLCD_DISP_CTRL2, 0x0302);   
  set_register(TFTLCD_DISP_CTRL3, 0x0000);  
  set_register(TFTLCD_DISP_CTRL4, 0x0008);
  //*******POWER CONTROL REGISTER INITIAL*******//    
  set_register(TFTLCD_POW_CTRL1, 0x0790);   
  set_register(TFTLCD_POW_CTRL2, 0x0005);   
  set_register(TFTLCD_POW_CTRL3, 0x0000);  
  set_register(TFTLCD_POW_CTRL4, 0x0000);   
  delay(50); 
  //********POWER SUPPPLY STARTUP 1 SETTING*******//    
  set_register(TFTLCD_POW_CTRL1, 0x12B0);   
  delay(50);
  set_register(TFTLCD_POW_CTRL2, 0x0007);   
  delay(50);
  //********POWER SUPPLY STARTUP 2 SETTING******//    
  set_register(TFTLCD_POW_CTRL3,  0x008C);   
  set_register(TFTLCD_POW_CTRL4,  0x1700);  
  set_register(TFTLCD_VCOMH_CTRL, 0x0022);  
  delay(50);
  //******GAMMA CLUSTER SETTING******//    
  set_register(TFTLCD_GAMMA_CTRL1, 0x0000);   
  set_register(TFTLCD_GAMMA_CTRL2, 0x0505);   
  set_register(TFTLCD_GAMMA_CTRL3, 0x0205);   
  set_register(TFTLCD_GAMMA_CTRL4, 0x0206);   
  set_register(TFTLCD_GAMMA_CTRL5, 0x0408);   
  set_register(TFTLCD_GAMMA_CTRL6, 0x0000);   
  set_register(TFTLCD_GAMMA_CTRL7, 0x0504);
  set_register(TFTLCD_GAMMA_CTRL8, 0x0206);    
  set_register(TFTLCD_GAMMA_CTRL9, 0x0206);  
  set_register(TFTLCD_GAMMA_CTRL10, 0x0408);   
  // enter full color mode
  write_command(0x38); 
  // -----------DISPLAY WINDOWS 240*320-------------//    
  set_register(TFTLCD_HOR_START_AD, 0x0000);
  set_register(TFTLCD_HOR_END_AD,   0x00EF);
  set_register(TFTLCD_VER_START_AD, 0x0000);   
  set_register(TFTLCD_VER_END_AD,   0x013F);
  //-----FRAME RATE SETTING-------//    
  set_register(TFTLCD_GATE_SCAN_CTRL1, 0xA700);  
  set_register(TFTLCD_GATE_SCAN_CTRL2, 0x0001);   
  set_register(TFTLCD_PANEL_IF_CTRL1, 0x0033); //RTNI setting
  //-------DISPLAY ON------//    
  set_register(TFTLCD_DISP_CTRL1, 0x0133);

  digitalWrite(LCD_RD,HIGH);
}


