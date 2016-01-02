

  void     interpolateFlood(uint16_t y, uint16_t i, uint16_t length, uint16_t color1, uint16_t color2);
  void     interpolateFastHLine(int16_t x0, int16_t y0, uint8_t w, uint16_t color1, uint16_t color2);
  void     shadeTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color0, uint16_t color1, uint16_t color2);
  uint16_t interpolate(uint16_t color1, uint16_t color2, int weight);

/*  Computes convex combination of color1 and color2 with weight
 *  Weight is normalizes s.t. [0,1] maps to [0,256]
 */
uint16_t Uno_TFTLCD::interpolate(uint16_t color1, uint16_t color2, int weight) {
  // The clean way: break out the RGB components and reassemble
  uint16_t alpha = weight;
  uint16_t beta  = 256-alpha;
  uint8_t r = (((color1>>11)&0b011111)*alpha + ((color2>>11)&0b011111)*beta) + 0b1111111 >> 8;
  uint8_t g = (((color1>> 5)&0b111111)*alpha + ((color2>> 5)&0b111111)*beta) + 0b1111111 >> 8;
  uint8_t b = (((color1    )&0b011111)*alpha + ((color2    )&0b011111)*beta) + 0b1111111 >> 8;
  return (((uint16_t)r)<<(5+6)) | (((uint16_t)g)<<(5)) | ((uint16_t)b);
}

// Horizontal fill of a segment with color interpolation
void Uno_TFTLCD::interpolateFlood(uint16_t y, uint16_t i, uint16_t length, uint16_t color1, uint16_t color2)
{
    START_PIXEL_DATA();
    uint8_t r1 = (color1>>11);
    uint8_t g1 = (color1>> 5)&0b111111;
    uint8_t b1 = (color1    )&0b011111;
    uint8_t r2 = (color2>>11);
    uint8_t g2 = (color2>> 5)&0b111111;
    uint8_t b2 = (color2    )&0b011111;
    for (; i<length; i++) {
        uint16_t weight = (i*64/length);
        uint8_t alpha = weight;
        uint8_t beta = 64-alpha;
        uint8_t r = r1*alpha + r2*beta >> 6;
        uint8_t g = g1*alpha + g2*beta >> 6;
        uint8_t b = b1*alpha + b2*beta >> 6;
        uint16_t color =  (((uint16_t)r)<<(5+6)) | (((uint16_t)g)<<(5)) | ((uint16_t)b);
        color = color&0b1111011111111111 | (mask_flag^(0b1000*(y&1)))*0x0100;
        SEND_PAIR(color>>8,color);
    }
}

// Fast horizontal line supporting overdraw and interpolation
// Does not support masking
void Uno_TFTLCD::interpolateFastHLine(int16_t x, int16_t y, uint8_t length, uint16_t color1, uint16_t color2) {
    if (length<1) return;
    int16_t x2;
    #ifdef DO_CLIP
        if((length<=0)||(y<0)||(y>= _height)||(x>=_width)||((x2=(x+length-1))<0)) return;
        if(x<0) {length+=x; x=0;}
        if(x2>=_width) {x2=_width-1; length=x2-x+1;}
    #endif    
    SET_Y_LOCATION(y);
    if (!do_overdraw) {
        SET_X_LOCATION(x);   
        interpolateFlood(y,0,length,color1,color2);
        return;
    }
    int in_segment=0;
    int start=x;
    int stop =x+length;
    int i=x;
    while (i<stop) {
        // Begin reading pixel information
        SET_X_LOCATION(i);
        START_READING();
        while (i<stop) {
            uint8_t read = PIND&0b11111100;
            uint8_t is_masked = (read&0b11110111)!=0 && (read&0b1000)==(mask_flag^(0b1000*(y&1)));
            PORTC=SEND_DATA;  // Read clock high (rising edge)
            PORTC=READY_READ; // Read clock low (falling edge)
            PORTC=SEND_DATA;  // Read clock high (rising edge)
            if (is_masked) {
                if (in_segment) {
                    STOP_READING();
                    SET_X_LOCATION(start);
                    interpolateFlood(y,start-x,i-x,color1,color2);
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
        interpolateFlood(y,start-x,i-x,color1,color2);
    }
}

// Shade a triangle with three colors
// Supports optional overdraw rendering
// Does not support masking -- masking is typically used to erases so
// will not be needed here.
void Uno_TFTLCD::shadeTriangle ( 
          int16_t x0, int16_t y0,
				  int16_t x1, int16_t y1,
				  int16_t x2, int16_t y2, 
          uint16_t color0, 
          uint16_t color1, 
          uint16_t color2) {
    int16_t a, b, y, last;
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); uint16_t temp = color0; color0 = color1; color1 = temp; }
    if (y1 > y2) { swap(y2, y1); swap(x2, x1); uint16_t temp = color2; color2 = color1; color1 = temp; }
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); uint16_t temp = color0; color0 = color1; color1 = temp; }
    if(y0 == y2)  return;
    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip the y1 scanline
    int len1 = y2 - y0;
    int len2 = y1 - y0;
    int len3 = y2 - y1;
    int round2 = len2/2;
    int round3 = len3/2;
    uint16_t midpoint = interpolate(color2,color0,(y1-y0)*256/(y2-y0));
    uint16_t colorX = midpoint;
    uint16_t colorY = color1;
    uint16_t colorZ = color0;
    // Skip the first line to avoid triangle overlap in meshes
    sa += dx01;
    sb += dx02;
    for(y=y0+1; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        float weight = ((y-y0)*256.0)/len2;
        uint16_t colorA = interpolate(colorX,colorZ,weight);
        uint16_t colorB = interpolate(colorY,colorZ,weight);
        if(a > b) {
            swap(a,b);
            swap(colorA,colorB);
        }
        interpolateFastHLine(a, y, b-a, colorA, colorB);
    }
    uint16_t colorU = midpoint;
    uint16_t colorV = color1;
    uint16_t colorW = color2;
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        float weight = 256-((y-y1)*256.0)/len3;
        uint16_t colorA = interpolate(colorU,colorW,weight);
        uint16_t colorB = interpolate(colorV,colorW,weight);
        if(a > b) {
            swap(a,b);
            swap(colorA,colorB);
        }
        interpolateFastHLine(a, y, b-a, colorA, colorB);
    }
}


