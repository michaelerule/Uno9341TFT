int8_t    button_selected = -1;

// Colors may change while the app is running so we draw them separately  
void redraw_color_selector_buttons() {
  const uint8_t color = 0b10010100;
  const uint8_t width = BUTTON_SIZE;
  uint8_t  x0,x1,dx,dy;
  uint16_t y0,y1;
  // Seventh button. Foreground color selector
  x0 = width*0+1;
  y0 = 320-width+2;
  dx = (width)/2-1;
  dy = (width)/2-2;
  tft.fastFillRect(x0,y0,dx,dy,RED);
  tft.fastFillRect(x0+dx,y0,dx,dy,GREEN);
  tft.fastFillRect(x0,y0+dy,dx,dy,BLUE);
  tft.fastFillRect(x0+dx,y0+dy,dx,dy,YELLOW);
  x0 = width*0+8;
  y0 = 320-width+8;
  dx = (width-16);
  dy = (width-16);
  tft.fastFillRect(x0,y0,dx,dy,tft.foreground_color&0b11111100);
  
  // Eighth button. Background color selector
  x0 = width*1;
  y0 = 320-width+1;
  dx = width;
  dy = width-1;
  tft.fastFillRect(x0,y0,dx,dy,tft.background_color&0b11111100);
  x0 = width*1+8;
  y0 = 320-width+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  dx = (width-16)/2;
  dy = (width-16)/2;
  tft.fastFillRect(x0,y0,dx,dy,RED);
  tft.fastFillRect(x0+dx,y0,dx,dy,GREEN);
  tft.fastFillRect(x0,y0+dy,dx,dy,BLUE);
  tft.fastFillRect(x0+dx,y0+dy,dx,dy,YELLOW);
}

/** 
 *  Render menu system for 3D model demo.
 *  12 buttons, 6 on each size. 40x40 pixels each
 */
void drawButtons() {
  tft.fastFillScreen(0);
  // Draw a button grid
  const uint8_t color = 0b10010100;
  const uint8_t width = BUTTON_SIZE;
  tft.fastestHLine(0,width,240,color);
  tft.fastestHLine(0,320-width,240,color);
  for (int i=1; i<6; i++) {
    tft.fastestVLine(i*width,0,width,color);
    tft.fastestVLine(i*width,0,width,color);
    tft.fastestVLine(i*width,320-width,width,color);
    tft.fastestVLine(i*width,320-width,width,color);
  }
  uint8_t  x0,x1,dx,dy;
  uint16_t y0,y1;
  // First button. Point cloud render
  x0 = 0+8;
  y0 = 0+10;
  for (int x=0; x<3; x++)
    for (int y=0; y<3; y++)
      tft.fastPixel(x0+x*width/4,y0+y*width/4,color);
  
  // Second button. Outline render.
  x0 = width+8;
  y0 = 0+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  tft.fastLine(x0,y0,x1,y0,color);
  tft.fastLine(x0,y0,x0,y1,color);
  tft.fastLine(x0,y1,x1,y1,color);
  tft.fastLine(x1,y0,x1,y1,color);
  
  // Thid button. Mesh render
  x0 = width*2+8;
  y0 = 0+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  tft.fastLine(x0,y0,x1,y0,color);
  tft.fastLine(x0,y0,x1,y1,color);
  tft.fastLine(x0,y0,x0,y1,color);
  tft.fastLine(x0,y1,x1,y1,color);
  tft.fastLine(x1,y0,x1,y1,color);

  // Fourth button. Solid render
  x0 = width*3+8;
  y0 = 0+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  tft.fastFillRect(x0,y0,width-16,width-16,color);

  // Fifth button. Fill render
  x0 = width*4+8;
  y0 = 0+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  tft.fastFillTriangle(x0,y0,x0,y1,x1,y0,color);
  tft.fastFillTriangle(x1,y1,x1,y0,x0,y1,0b11110100);

  // Sixth button. Shaded render
  x0 = width*5+8;
  y0 = 0+8;
  x1 = x0+width-16;
  y1 = y0+width-16;
  for (uint8_t i=0; i<width-16; i++)
    tft.interpolateFastHLine(x0,y0+i,width-16,0,15);

  redraw_color_selector_buttons();

  // Ninth button. Toggle color modes. 
  x0 = width*2+2;
  y0 = 320-width+3;
  dx = (width)-3;
  dy = (width)/3-1;
  tft.fastFillRect(x0,y0,dx,dy-2,RED);
  tft.fastFillRect(x0,y0+dy,dx,dy-2,GREEN);
  tft.fastFillRect(x0,y0+dy*2,dx,dy-2,BLUE);

  // Tenth button. auto rotate mode 
  x0 += width;
  dx = (width)/2-2;
  tft.drawCircle(x0+dx,y0+dx,width/2-8,color*0x0101);
}

void hilight_button(int8_t button) {
    if (button<1 || button>12) return;
    if (button<=6) 
        tft.fastXORRect((button-1)*BUTTON_SIZE,0,BUTTON_SIZE,BUTTON_SIZE,0xff);
    else
        tft.fastXORRect((button-7)*BUTTON_SIZE,320-BUTTON_SIZE,BUTTON_SIZE,BUTTON_SIZE,0xff);
}

void buttonPointCloudClicked() {rendering_mode = POINT_CLOUD_RENDERING_MODE;}// clearDrawing();}
void buttonOutlineClicked()    {rendering_mode = EDGE_RENDERING_MODE;       }// clearDrawing();}
void buttonMeshClicked()       {rendering_mode = MESH_RENDERING_MODE;       }// clearDrawing();}
void buttonSilhouetteClicked() {rendering_mode = SILHOUETTE_RENDERING_MODE; }// clearDrawing();}
void buttonFacetClicked()      {rendering_mode = FACET_RENDERING_MODE;      }// clearDrawing();}
void buttonShadedClicked()     {rendering_mode = SHADED_RENDERING_MODE;     }// clearDrawing();}

void buttonBackgroundColorClicked() {
  color_choosing_mode = 1;
  drawFastColorPallet();
}

void buttonForegroundColorClicked() {
  color_choosing_mode = 2;
  drawFastColorPallet();
}

void toggleColorMode() {tft.toggle_low_color_mode();}

void doRotateBenchmark() {
  rotation_benchmark_start = micros();
  rotation_benchmark_counter = 16;
}

#define NBUTTONS 10

void (*button_events[NBUTTONS])() = {
  &buttonPointCloudClicked,
  &buttonMeshClicked,
  &buttonOutlineClicked,
  &buttonSilhouetteClicked,
  &buttonFacetClicked,
  &buttonShadedClicked,
  &buttonForegroundColorClicked,
  &buttonBackgroundColorClicked,
  &toggleColorMode,
  &doRotateBenchmark};

void button_event() {
    hilight_button(button_selected); 
    if (button_selected>=1&&button_selected<=NBUTTONS)
      button_events[button_selected-1]();
    button_selected=-1;
}

void button_touch() {
    int8_t button = -1;
    if (touch_y<=BUTTON_SIZE) 
        button = touch_x/BUTTON_SIZE+1;
    else if (touch_y>=320-BUTTON_SIZE)
        button = touch_x/BUTTON_SIZE+7;
    if (button!=button_selected) {
        hilight_button(button);
        hilight_button(button_selected);
        button_selected=button;
    }
}

