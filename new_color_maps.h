#define NCOLORS 16

PROGMEM const uint8_t reddish[NCOLORS]  = { 
    0b00100000, 0b01000000, 0b01100000, 0b10000000, 0b10100000, 0b11000000, 0b11100000, 0b11100000, 0b11100001, 0b11100010, 0b11100011, 0b11100100, 0b11100101, 0b11100110, 0b11100111, 0b11110111, }; 
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG

PROGMEM const uint8_t greenish[NCOLORS] = { 
    0b00100000, 0b00100001, 0b00100010, 0b00100011, 0b00000100, 0b00000101, 0b00000110, 0b00000111, 0b00100111, 0b01000111, 0b01100111, 0b10000111, 0b10100111, 0b11000111, 0b11100111, 0b11110111, }; 
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG

PROGMEM const uint8_t greyish[NCOLORS]  = { 
    0b00000000, 0b00000000, 0b00000000, 0b00100000, 0b00100000, 0b00100000, 0b00100001, 0b01000001, 0b01010001, 0b01010010, 0b01110011, 0b10010100, 0b10110101, 0b11010110, 0b11110111, 0b11110111, }; 
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG

PROGMEM const uint8_t blueish[NCOLORS]  = { 
    0b00100000, 0b01001001, 0b00010000, 0b01010010, 0b00110011, 0b00110100, 0b00010101, 0b00010110, 0b00010111, 0b00110111, 0b01010111, 0b01110111, 0b10010111, 0b10110111, 0b11010111, 0b11110111, }; 
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG

PROGMEM const uint8_t pinkish[NCOLORS]  = { 
    0b00000000, 0b00100000, 0b01000000, 0b01000001, 0b01000010, 0b00010000, 0b00010011, 0b00110011, 0b01010100, 0b01110100, 0b10010100, 0b10010100, 0b10110100, 0b11010100, 0b11110100, 0b11110100, }; 
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG
  
PROGMEM const uint8_t brownish[NCOLORS] = { 
    0b00000000, 0b00100000, 0b01000000, 0b01000001, 0b01000010, 0b01000011, 0b01100011, 0b10000011, 0b10100011, 0b11000011, 0b11100011, 0b11100100, 0b11100101, 0b11100110, 0b11100111, 0b11110111, };  
//    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB    GGGBBBBB  
//    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG    RRRRRGGG

