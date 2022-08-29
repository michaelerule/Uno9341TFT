#include <math.h>
#include "Arduino_3D.h" // Hardware-specific library

// INCLUDE MODEL DATA
#include "sphere.h"
//#include "bunny.h"
//#include "small_bunny.h"
//#include "face.h"

#define AXLEN 0.4

Arduino_3D tft;

#define POINT_CLOUD_RENDERING_MODE 0
#define EDGE_RENDERING_MODE        1
#define MESH_RENDERING_MODE        2
#define SILHOUETTE_RENDERING_MODE  3
#define FACET_RENDERING_MODE       4
#define SHADED_RENDERING_MODE      5
uint8_t rendering_mode = MESH_RENDERING_MODE;

// Just rotate all the time; useful demo if touchscreen input is not working
#define AUTOROTATE 1

int16_t   touch_x,touch_y,touch_z;
uint8_t   color_choosing_mode = 0;
unsigned  long rotation_benchmark_start;
int16_t   rotation_benchmark_counter = 0;
#define   BUTTON_SIZE 40

void clearDrawing() { 
  tft.masking_off();
  tft.fastFillRect(0,BUTTON_SIZE+1,240,240-1,tft.background_color); 
}

#include "color_picker.h"
#include "buttons.h"
#include "touch_routines.h"

void setup() {
  tft.begin();
  tft.fillScreen(BLACK);
  pinMode(13, OUTPUT);
  drawButtons();
}

void loop() {
  tft.background_color = BLACK;
  clearDrawing();
  model(); // does not return
}

// need a little more contrast -- draw vertices twice
// once in foreground and once in inverted foreground?
void draw_bold_vertices(Model *M, int8_t *vbuff) {
    tft.foreground_color ^= 0xffff;
    tft.X0--;
    tft.drawVertices(M, vbuff);
    tft.X0++;
    tft.foreground_color ^= 0xffff;
    tft.drawVertices(M, vbuff);
}
void erase_bold_vertices(Model *M, int8_t *vbuff) {
    tft.X0--;
    tft.eraseVertices(M,vbuff);
    tft.X0++;
    tft.eraseVertices(M,vbuff);
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
  tft.applyTransform(M,abuff1,vbuff2);

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
    
    float dx=0,dy=0;
    if (AUTOROTATE||rotation_benchmark_counter>1) {
        // perform rotation benchmark
        dy = 0.005;
        dx = 0.011;
        rotation_benchmark_counter--;
    } else if (rotation_benchmark_counter==1) {
        // Finish the benchmark: estimate frame rate and print it
        int fps = 16000000L / (micros() - rotation_benchmark_start);
        tft.setCursor(5, 5+BUTTON_SIZE);
        tft.setTextColor(tft.foreground_color,tft.background_color);
        tft.setTextSize(1);
        tft.print(fps);
        tft.print("   ");
        // Wrap things up.
        rotation_benchmark_counter=0;
        // patch to get the last frame to render
        active_rendering=1;
    } else {
        // Get user input
        poll_touch();
        dx = DX;
        dy = DY;
    }

    // If there is a sufficient change, update the model
    if (active_rendering || dx<-MIND || dx>MIND || dy<-MIND || dy>MIND) {
        tft.rotateTransformXY(abuff1,dx,dy,abuff2);
        {float *temp2 = abuff1; abuff1 = abuff2; abuff2 = temp2;}
        tft.applyTransform(M,abuff1,vbuff1);
        {int8_t *temp = vbuff1; vbuff1 = vbuff2; vbuff2 = temp;}
    }
    
    // User input mode: render quickly
    if (model_drag) {
      // Just draw points for speed
      draw_bold_vertices(M, vbuff2);
      // Depending on whether the previous frame was rendered
      // Quickly or not, either erase points or erase the bounding
      // box.
      if (active_rendering==0) tft.eraseBoundingBox(M,vbuff1);
      else erase_bold_vertices(M,vbuff1);
      active_rendering = 1;
      tft.flip_mask();
    }
    
    // Static render: render nicely
    else if (rotation_benchmark_counter || active_rendering || previous_rendering_mode != rendering_mode) {
        rendering_mode &= 0b1111111;
        switch (rendering_mode) {
            case POINT_CLOUD_RENDERING_MODE:
                draw_bold_vertices(M, vbuff2);
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
            // Leonardo is too small and slow to support shaded rendering
            // So this is enabled only for the Uno
            #ifndef SAVE_SPACE  
            case SHADED_RENDERING_MODE: {
                  uint8_t normal_colors[NVERTICES];
                  tft.computeVertexLightingColors(M,abuff1,normal_colors);
                  tft.shadeFaces(M,vbuff2,normal_colors,permutation);
                }
                break;
            #endif
            default: ;
        }
        // Use the fastest erase method depending on the content of the 
        // previous frame
        switch (previous_rendering_mode) {
            case POINT_CLOUD_RENDERING_MODE:
                erase_bold_vertices(M, vbuff1);
                break;
            case EDGE_RENDERING_MODE:
                tft.eraseEdges(M, vbuff1);
                break;
            case MESH_RENDERING_MODE:
                tft.eraseMesh(M, vbuff1);
                break;
            default: tft.eraseBoundingBox(M,vbuff1);
        }
        
        active_rendering = 0;
        previous_rendering_mode = rendering_mode;
        tft.flip_mask();
    }
  }
}


