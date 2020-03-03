#define color565(r, g ,b) ((((r)&0xF8)<<8)|(((g)&0xFC)<<3)|((b)>>3))

// Assign human-readable names to some common 16-bit color values:
// Using values with identical high and low bytes gives faster
// Rendering of filled regions
#define BLACK   0b0000000000000000
#define BLUE    0b0001000000010000
#define RED     0b1110000011100000
#define GREEN   0b0000010000000100
#define CYAN    0b0001010000010100
#define PURPLE  0b0111000001110000
#define MAGENTA 0b1111000011110000
#define LIME    0b0110011101100111
#define YELLOW  0b1110011011100110
#define ORANGE  0b1110010011100100
#define WHITE   0b1111011111110111
#define GREY    0b0110001101101011
#define MASK   ~0b1111011111110111
