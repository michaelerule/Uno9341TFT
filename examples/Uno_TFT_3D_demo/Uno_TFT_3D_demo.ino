#include <math.h>
#include <Uno_3D.h> // Hardware-specific library

// INCLUDE MODEL DATA
//#include "sphere.h"
//#include "small_bunny.h"
//#include "bunny.h"
#include "face.h"

#define AXLEN 0.5

Uno_3D tft(A3,A2,A1,A0,A4);

#define POINT_CLOUD_RENDERING_MODE 0
#define EDGE_RENDERING_MODE        1
#define MESH_RENDERING_MODE        2
#define SILHOUETTE_RENDERING_MODE  3
#define FACET_RENDERING_MODE       4
#define SHADED_RENDERING_MODE      5
uint8_t rendering_mode = 4;

int16_t   touch_x,touch_y,touch_z;
uint8_t   color_choosing_mode = 0;
unsigned long rotation_benchmark_start;
int16_t   rotation_benchmark_counter = 0;
#define   BUTTON_SIZE 40
#include "color_picker.h"
#include "buttons.h"
#include "touch_routines.h"

void setup() {
  tft.reset();
  tft.begin(0x9341);
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  drawButtons();
  set_foreground_color_index(6);
}

void clearDrawing() { 
  tft.masking_off();
  tft.fastFillRect(0,BUTTON_SIZE+1,240,240-1,tft.background_color); 
}

void loop() {
  tft.background_color = BLACK;
  clearDrawing();
  model(); // does not return
}

void model() {

  init_model();
  Model *M = &model_data;
  
  // Rotated versions of the points are buffered here
  int8_t buff[NVERTICES*2*3];
  int8_t *vbuff1 = &buff[0];
  int8_t *vbuff2 = &buff[NVERTICES*3];

  // Define 3D axis. We rotate this based on touch input
  float axis[4*2*3];
  float *abuff1 = &axis[0];
  float *abuff2 = &axis[4*3]; 
  tft.getScaleTransform(AXLEN,abuff1);
  tft.applyTransform(M,abuff1,vbuff1);
  {int8_t *temp = vbuff1; vbuff1 = vbuff2; vbuff2 = temp;}

  // Initialize permutation. We keep this across frames
  // Because sorting a mostly ordered set is faster. 
  uint8_t permutation[NTRIANGLES];
  for (uint16_t i=0; i<NTRIANGLES; i++) permutation[i]=i;

  uint8_t previous_rendering_mode = rendering_mode;
  uint8_t active_rendering = 1;
  
  while (1) 
  {
    if (color_choosing_mode) {
        poll_touch();
        continue;
    }
    
    float dx,dy;
    if (rotation_benchmark_counter>1) {
        // perform rotation benchmark
        dy = 0.393;
        dx = 0;
        rotation_benchmark_counter--;
    } else if (rotation_benchmark_counter==1) {
        int fps = 16000000L / (micros() - rotation_benchmark_start);
        tft.setCursor(5, 5+BUTTON_SIZE);
        tft.setTextColor(tft.foreground_color,tft.background_color);
        tft.setTextSize(1);
        tft.print(fps);
        tft.print("   ");
        rotation_benchmark_counter=0;
    } else {
        // Get user input
        poll_touch();
        dx = DX;
        dy = DY;
    }

    // If there is a sufficient change, update the model
    if (dx<-MIND || dx>MIND || dy<-MIND || dy>MIND) {
        tft.rotateTransformXY(abuff1,dx,dy,abuff2);
        {float *temp2 = abuff1; abuff1 = abuff2; abuff2 = temp2;}
        tft.applyTransform(M,abuff1,vbuff1);
        {int8_t *temp = vbuff1; vbuff1 = vbuff2; vbuff2 = temp;}
    }
    
    if (model_drag) {
      // Just draw points for speed
      tft.drawEdges(M, vbuff2);
      if (active_rendering==0) tft.eraseBoundingBox(M,vbuff1);
      else tft.eraseEdges(M, vbuff1);
      active_rendering = 1;
      tft.flip_mask();
    }
    
    else if (rotation_benchmark_counter || active_rendering || previous_rendering_mode != rendering_mode) {
        rendering_mode &= 0b1111111;
        switch (rendering_mode) {
            case POINT_CLOUD_RENDERING_MODE:
                tft.drawVertices(M, vbuff2);
                break;
            case EDGE_RENDERING_MODE:
                tft.drawEdges(M, vbuff2);
                break;
            case MESH_RENDERING_MODE:
                tft.drawMesh(M, vbuff2);
                break;
            case SILHOUETTE_RENDERING_MODE:
                tft.fillFaces(M,vbuff2,NULL,NULL); 
                break;
            case FACET_RENDERING_MODE: {
                  uint8_t face_colors[NTRIANGLES];
                  tft.computeFaceLightingColors(M,abuff1,face_colors);
                  tft.fillFaces(M,vbuff2,face_colors,permutation); 
                }
                break;
            case SHADED_RENDERING_MODE: {
                  uint8_t normal_colors[NVERTICES];
                  tft.computeVertexLightingColors(M,abuff1,normal_colors);
                  tft.shadeFaces(M,vbuff2,normal_colors,permutation);
                }
                break;
            default: ;
        }
        /*
        switch (previous_rendering_mode*(!active_rendering)) {
            case POINT_CLOUD_RENDERING_MODE:
                tft.eraseVertices(M,vbuff1);
                break;
            case MESH_RENDERING_MODE:
                tft.eraseMesh(M,vbuff1);
                break;
            default: 
                tft.eraseBoundingBox(M,vbuff1);
        }
        */
                tft.eraseBoundingBox(M,vbuff1);
        
        active_rendering = 0;
        previous_rendering_mode = rendering_mode;
        tft.flip_mask();
    }
  }
}


