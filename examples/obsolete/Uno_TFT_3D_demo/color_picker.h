#define PALLET_SIZE 40
#define PALLET_X ((240-4*PALLET_SIZE)/2)
#define PALLET_Y ((320-3*PALLET_SIZE)/2)

int color_selected = -1;

// python command to generate these integers
// pp = [14,15,9,3,17,16,24,30,6,0,25,31]
// print [(((j>>2)&0b100) | ((j<<4)&0b11110000)) for j in pp]
uint8_t pallet[12] = {224, 240, 144, 48, 20, 4, 132, 228, 96, 0, 148, 244};
    
/** In fast mode only 5 bits of color data are available. 
 *  High and Low bytes are constrained to be identical.
 *  Two bits are discarded to avoid a PORTB write
 *  One bit is reserved to keep track of masking and overdraw.
 *  These are thie bits that are used
 *  0b11110100
 */
void drawFastColorPallet() {  
    tft.masking_off();
    tft.fastFillRect(0,BUTTON_SIZE+1,240,240-1,BLACK);
    for (int i=0; i<12; i++) {
        uint8_t c = pallet[i];       
        tft.fastFillRect((i/3)*PALLET_SIZE+PALLET_X+2,(i%3)*PALLET_SIZE+PALLET_Y+2,PALLET_SIZE-4,PALLET_SIZE-4,c);
    }
    for (int i=PALLET_Y; i<PALLET_Y+PALLET_SIZE*3; i+=2) 
        tft.fastXORRect(PALLET_X,i,PALLET_SIZE*4,1,0b00001000);
}

void hilight_color(int color) {
    if (color<0 || color>=12) return;
    tft.fastXORRect((color/3)*PALLET_SIZE+PALLET_X,(color%3)*PALLET_SIZE+PALLET_Y,PALLET_SIZE,PALLET_SIZE,0xff);
}

void color_chooser_touch() {
  int selected = -1;
  int y = touch_y - PALLET_Y;
  int x = touch_x - PALLET_X;
  if (y>=0 && y<PALLET_SIZE*3
   && x>=0 && x<PALLET_SIZE*4) {
    selected = (y/PALLET_SIZE) + 3*(x/PALLET_SIZE);
  }
  if (selected!=color_selected) {
      hilight_color(selected);
      hilight_color(color_selected);
      color_selected=selected;
  }
}

void redraw_color_selector_buttons();

void set_foreground_color_index(uint8_t color_selected) {
    tft.foreground_color = pallet[color_selected]*0x0101;
    tft.setColorMap(color_selected);
}

// should close color chooser
void color_chooser_event() {
    if (color_selected>=0 && color_selected<12) {
        hilight_color(color_selected);
        if (color_choosing_mode==2) set_foreground_color_index(color_selected);
        if (color_choosing_mode==1) tft.background_color = pallet[color_selected]*0x0101;
        color_choosing_mode = 0;
        clearDrawing();
        redraw_color_selector_buttons();
        rendering_mode |= 0b10000000;
    }
    color_selected=-1;
}

