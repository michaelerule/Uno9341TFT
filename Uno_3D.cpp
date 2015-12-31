/**
 * Extends the UnoTFTLCD class with 3D drawing routines.
 * The base classes UnoTFTLCD and UnoGFX have been modified to support 
 * the specific drawing routines required for 3D drawing.
 * 
 * All drawing commands accept optional list of colors for the elements.
 * If list is NULL then the foreground color is used by default. 
 *
 * Triangle lists, edge lists, colors, and edge maps are to be stored in 
 * Pram memory (PMEM). tris and edges are stored as tiplets and
 * pairs of uint8_t indecies into vertex arrays. Edge maps additionally 
 * store two uint8_t indecies into the triangle array. Colors are uint16_t
 * lists. 
 * 
 * Vertex lists for drawing are to be stored in RAM. Edge sets for drawing
 * are to be store in RAM. 
 * 
 * Raw vertex sets (before geometry transformaton) are to be stored in 
 * Pram memory (PMEM) as int8_t lists. 
 */

#include <avr/pgmspace.h>
#include "pins_arduino.h"
#include <math.h>
#include <stdint.h>
#include "Uno_3D.h"

#define readVertexCoordinate(v) ((float)((int8_t)pgm_read_byte(&(v))))
#define readSignedByte(v)       ((int8_t)pgm_read_byte(&(v)))
#define readUnsignedByte(t)     ((uint8_t)pgm_read_byte(&(t)))
#define readColor(c)            ((uint16_t)pgm_read_word(&(c)))

////////////////////////////////////////////////////////////////////////
// Constructors and routines for controlling state
////////////////////////////////////////////////////////////////////////

Uno_3D::Uno_3D(uint8_t cs, uint8_t cd, uint8_t wr, uint8_t rd, uint8_t rst) 
    : Uno_TFTLCD(cs,cd,wr,rd,rst) {
        setColorMap(11);
}

void Uno_3D::setLocation(uint16_t x0, uint16_t y0) {
    X0 = x0;
    Y0 = y0;
}

#include "color_maps.h"
void Uno_3D::setColorMap(uint8_t cmap) {
    PU8 map;
    switch (cmap) {
        case 0: map = &reddish[0]; break;
        case 1: 
        case 2: map = &pinkish[0]; break;
        case 3:
        case 4: map = &blueish[0]; break;
        case 5:
        case 6: map = &greenish[0]; break;
        case 7:
        case 8: map = &brownish[0]; break;
        case 9:
        case 10:
        case 11:
        default:
            map = &greyish[0];
    }
    for (uint8_t i=0; i<NCOLORS; i++) color_map[i] = readUnsignedByte(map[i]);
}

void Uno_3D::eraseRegion(uint8_t x0, uint16_t y0, uint8_t x1, uint16_t y1) {
    if (x1<x0 || y1<y0) return;    
	masking_on();
	uint8_t w = x1-x0+1;
	for (int i=y0; i<=y1; i++)
	    drawFastHLine(x0,i,w,background_color);
    	masking_off();
}

void Uno_3D::eraseBoundingBox(Model *M, int8_t *vertices) {
    uint16_t nv = (M->NVertices);
    int8_t maxx = -128;
    int8_t minx =  127;
    int8_t maxy = -128;
    int8_t miny =  127;
    for (int i=0; i<nv; i++) {
        int8_t x = vertices[i*3];
        if (x<minx) minx=x;
        if (x>maxx) maxx=x;
        int8_t y = vertices[i*3+1];
        if (y<miny) miny=y;
        if (y>maxy) maxy=y;
    }
    eraseRegion(X0+minx-1,Y0+miny-1,X0+maxx+1,Y0+maxy+1);
}
  
////////////////////////////////////////////////////////////////////////
// Main 3D drawing commands. Can draw points, meshes as specified by
// edges or by triangles, filled areas using solid colors, or shaded
// using face or vertex normals. 
////////////////////////////////////////////////////////////////////////

void Uno_3D::drawVertices(Model *M, int8_t *vertices, uint16_t color) {
    uint16_t nv = (M->NVertices);
    if (!do_masking) color = foreground_color;
    for (int i=0; i<nv; i++) 
        drawPixel(vertices[i*3]+X0,vertices[i*3+1]+Y0,color);
}
void Uno_3D::drawVertices(Model *M, int8_t *vertices) {
	drawVertices(M,vertices,foreground_color);
}
void Uno_3D::eraseVertices(Model *M, int8_t *vertices) {
	masking_on();
	drawVertices(M,vertices,background_color);
    	masking_off();
}

void Uno_3D::drawEdges(Model *M, int8_t *vertices) {
    drawEdges(M,vertices,foreground_color);
}

void Uno_3D::drawEdges(Model *M, int8_t *vertices, uint16_t color) {
    if (M->edges==NULL) {
        drawMesh(M,vertices,color);
        return;
    }
    uint16_t ne = M->NEdges;
    for (int i=0; i<ne; i++) {
        PU8 e = &M->edges[i*2];
        uint8_t a = readUnsignedByte(e[0]);
        uint8_t b = readUnsignedByte(e[1]);
        int8_t *p = &vertices[a*3];
        int8_t *q = &vertices[b*3];
        if (do_masking)
        drawLine(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,color);
        else 
        fastLine(p[0]+X0,p[1]+Y0,q[0]+X0,q[1]+Y0,color);
    }
}
void Uno_3D::eraseEdges(Model *M, int8_t *vertices) {
    masking_on();
    drawEdges(M, vertices, background_color);
    	masking_off();
}


////////////////////////////////////////////////////////////////////////
// face_colors may be NULL, to use the model-specified colors, or the 
// current foreground color if those are not available.
// set dashed to 0 to draw solid lines. If nonzero, it behaves
// like a bitmask, and only points numbers that mask to 0 are drawn
////////////////////////////////////////////////////////////////////////

void get_triangle_points(Model *M, int8_t *vertices, uint8_t i, int8_t **p, int8_t **q, int8_t **r) {
    // Get the vertex indecies for the triangle
    PU8 t = &M->faces[i*3];
    uint8_t pi = readUnsignedByte(t[0]);
    uint8_t qi = readUnsignedByte(t[1]);
    uint8_t ri = readUnsignedByte(t[2]);
    // get the  X and Y coordinates for the triangle
    *p = &vertices[pi*3];
    *q = &vertices[qi*3];
    *r = &vertices[ri*3];
}

uint8_t facing_camera(int8_t *p, int8_t *q, int8_t *r) {
    return (int)(r[0]-p[0])*(q[1]-p[1])<(int)(q[0]-p[0])*(r[1]-p[1]);
}
void Uno_3D::drawMesh(Model *M, int8_t *vertices) {
    drawMesh(M,vertices,foreground_color);
}
void Uno_3D::drawMesh(Model *M, int8_t *vertices, uint16_t color) {
    uint16_t nt    = M->NFaces;
    for (int i=0; i<nt; i++) {
        int8_t *p,*q,*r;
        get_triangle_points(M,vertices,i,&p,&q,&r);
        if (facing_camera(p,q,r)) {
            if (do_masking)
            drawTriangle(p[0]+X0,p[1]+Y0,
                         q[0]+X0,q[1]+Y0,
                         r[0]+X0,r[1]+Y0,
                         color);
            else
            fastDrawTriangle(p[0]+X0,p[1]+Y0,
                         q[0]+X0,q[1]+Y0,
                         r[0]+X0,r[1]+Y0,
                         color);
        }
    }
}
void Uno_3D::eraseMesh(Model *M, int8_t *vertices) {
    // Erase triangle using the masking feature
    masking_on();
    drawMesh(M,vertices,background_color);
    	masking_off();
}


////////////////////////////////////////////////////////////////////////
// Either face_colors or vertex_colors may be NULL, to use the model-
// specified colors, or the current foreground color if those are not
// available.
// Draw order may be NUL, but if it is provided triangles are sorted
// from front to back and overdraw avoidance is used. 
////////////////////////////////////////////////////////////////////////

void Uno_3D::fillFaces(Model *M, int8_t *vertices, uint8_t *face_colors, uint8_t *draw_order) {
    updateDrawingOrder(M,vertices,draw_order);
    uint16_t color = foreground_color;
    uint16_t nt    = M->NFaces;
    for (int j=0; j<nt; j++) {
        int i = draw_order? draw_order[j] : j;
        int8_t *p,*q,*r;
        get_triangle_points(M,vertices,i,&p,&q,&r);
        if (facing_camera(p,q,r)) {
            if (face_colors  !=NULL) color = color_map[face_colors[i]]*0x0101;
            uint8_t  x1 = p[0]+X0;
            uint8_t  x2 = q[0]+X0;
            uint8_t  x3 = r[0]+X0;
            //if (do_masking || do_overdraw)
            fillTriangle(x1,p[1]+Y0,x2,q[1]+Y0,x3,r[1]+Y0,color);
            //else
            //fastFillTriangle(x1,p[1]+Y0,x2,q[1]+Y0,x3,r[1]+Y0,color);
        }
    }
}
void Uno_3D::shadeFaces(Model *M, int8_t *vertices, uint8_t *vertex_colors, uint8_t *draw_order) {
    updateDrawingOrder(M,vertices,draw_order);
    uint16_t nt    = M->NFaces;
    for (int j=0; j<nt; j++) {
        int i = draw_order? draw_order[j]:j;
        // Get the vertex indecies for the triangle
        PU8 t = &M->faces[i*3];
        uint8_t pi = readUnsignedByte(t[0]);
        uint8_t qi = readUnsignedByte(t[1]);
        uint8_t ri = readUnsignedByte(t[2]);
        // get the vertex X and Y coordinates for the triangle
        int8_t *p = &vertices[pi*3];
        int8_t *q = &vertices[qi*3];
        int8_t *r = &vertices[ri*3];
        // if triangle is facing the camera, draw it
        if (facing_camera(p,q,r)) {
            uint8_t color1 = vertex_colors[pi];
            uint8_t color2 = vertex_colors[qi];
            uint8_t color3 = vertex_colors[ri];
            uint8_t  x1 = p[0]+X0;
            uint8_t  x2 = q[0]+X0;
            uint8_t  x3 = r[0]+X0;
            shadeTriangle(x1,p[1]+Y0,x2,q[1]+Y0,x3,r[1]+Y0,color1,color2,color3);
        }
    }
}

////////////////////////////////////////////////////////////////////////
// Routines for creating, rotating, and applying axis transformations 
////////////////////////////////////////////////////////////////////////
void Uno_3D::getScaleTransform(float AXLEN, float *abuff1) {
  for (uint8_t i=0; i<9; i++) abuff1[i]=0;
  abuff1[0] = abuff1[4] = abuff1[8] = AXLEN;
}
void Uno_3D::rotateTransformXY(float *input_transform, float dx, float dy, float *output_transform) {
    float cdx = cos(dx);
    float sdx = sin(dx);
    float cdy = cos(dy);
    float sdy = sin(dy);
    for (int j=0; j<3; j++) {
        float *a = &input_transform[j*3];
        float *b = &output_transform[j*3];
        float x = a[0];
        float y = a[1];
        float z = a[2];
        float nz = cdx*z - sdx*x;      
        b[0] = cdx*x  + sdx*z;
        b[1] = cdy*y  + sdy*nz;
        b[2] = cdy*nz - sdy*y;
    }
}

void transformPoint(float *transform,P8 p,int8_t *q) {
    int8_t nx = readSignedByte(p[0]);
    int8_t ny = readSignedByte(p[1]);
    int8_t nz = readSignedByte(p[2]);
    for (uint8_t i=0; i<3; i++)
        q[i] = nx*transform[i]+ny*transform[i+3]+nz*transform[i+6];
}

int8_t getZNormal(float *transform, P8 normal) {
    int8_t p[3];
    transformPoint(transform,normal,&p[0]);
    return p[2];
}

void Uno_3D::applyTransform(Model *M, float *transform, int8_t *vertices) {
    uint16_t nv = M->NVertices;
    for (int i=0; i<nv; i++) 
        transformPoint(transform,&M->vertices[i*3],&vertices[i*3]);
}

inline float inverseMagnitude(float x, float y, float z) {
    return 1.0/sqrt(x*x+y*y+z*z);
}
void normalizeTransform(float *transform, float *output) {
    for (uint8_t j=0; j<3; j++) {
        float scale = (NCOLORS/127.0)*inverseMagnitude(transform[j],transform[j+3],transform[j+6]);
        for (uint8_t i=j; i<9; i+=3) output[i] = transform[i]*scale;
    }
}

////////////////////////////////////////////////////////////////////////
// Routines for generating vertex and face colors 
// from lights or from depth-shading
////////////////////////////////////////////////////////////////////////
void project_normals(float *transform, P8 normals,uint16_t n,uint8_t *output) {
    float normalized[9];
    normalizeTransform(transform,normalized);
    for (uint16_t i=0; i<n; i++) {
        int8_t z = getZNormal(normalized,&normals[i*3]);
        output[i] = z<0?0:z;
    }
}
void Uno_3D::computeVertexLightingColors(Model *M, float *transform, uint8_t *vertex_colors) {
    project_normals(transform,M->vertexNormals,M->NVertices,vertex_colors);
}
void Uno_3D::computeFaceLightingColors(Model *M, float *transform, uint8_t *face_colors) {
    project_normals(transform,M->faceNormals,M->NFaces,face_colors);
}


////////////////////////////////////////////////////////////////////////
// Non-convex 3D surfaces can overlap themselvs. Sorting triangles from
// front to back and checking to make sure we don't draw on top of areas
// that have already been drawn can avoid Uno_3D::overlap artefacts. To 
// maintain sorted lists of polygons across frames, theses functions
// accept a permutation list for the triangle drawing order. The 
// permutation is updated to reflect the current z-order. 
////////////////////////////////////////////////////////////////////////

// Helper routine for sorting triangles
// Faces may remain partially ordered after rotating the object
// To take advantage of this, we store and use a fixed permutation 
// array "draw_order"

void Uno_3D::computeTriangleDepths(Model *M, int8_t *vertices, uint8_t *draw_order, uint8_t *depths) {
    uint16_t nt = M->NFaces;
    for (int j=0; j<nt; j++) {
        int i = draw_order!=NULL? draw_order[j]:j;
        int8_t *p,*q,*r;
        get_triangle_points(M,vertices,i,&p,&q,&r);
        // get the rotated vertex Z coordinates for the triangle
        int8_t z = (p[2]+q[2]+r[2])/3;
        depths[j] = z;
    }
}
// Sorts triangles from front to back to properly handle occlusions
// Bubble sort is in fact the efficient solution here. 
// It is O(N) for sorted data, and requires no additional memory to sort. 
// Triangles remain mostly sorted as object rotates.
void Uno_3D::updateDrawingOrder(Model *M, int8_t *vertices, uint8_t *draw_order) {
    if (draw_order==NULL) return;
    uint16_t nt = M->NFaces;
    uint8_t depths[nt];
    computeTriangleDepths(M,vertices,draw_order,depths);
    // Bubble sort the triangles by depth keeping track of the permutation
    uint8_t sorted = 0;
    while (!sorted) {
        sorted = 1;
        for (int i=1;i<nt;i++) {
            int8_t d1 = depths[i-1];
            int8_t d2 = depths[i];
            if (d2>d1) {
                depths[i-1] = d2;
                depths[i]   = d1;
                uint8_t temp    = draw_order[i];
                draw_order[i]   = draw_order[i-1];
                draw_order[i-1] = temp;
                sorted = 0;
            }
        }
    }
    overdraw_on();
}


////////////////////////////////////////////////////////////////////////
// Triangle shaders
////////////////////////////////////////////////////////////////////////


/*  Computes convex combination of color1 and color2 with weight
 *  Weight is normalizes s.t. [0,1] maps to [0,256]
 */
uint16_t Uno_3D::interpolate(uint8_t color1, uint8_t color2, uint8_t alpha) {
  // The clean way: break out the RGB components and reassemble
  return color1 * alpha + color2 * (32-alpha) >> 5;
}

// Horizontal fill of a segment with color interpolation
void Uno_3D::interpolateFlood(uint16_t y, uint16_t i, uint16_t stop, uint16_t length, uint8_t color1, uint8_t color2)
{
    uint8_t flag = (mask_flag^(0b1000*(y&1)));
    uint8_t alpha = i*32/length;
    START_PIXEL_DATA();
    PORTD=PORTB=color_map[color1*alpha + color2*(32-alpha) >> 5] | flag;
    while (i<stop) {
        CLOCK_1;
        i++;
        uint8_t weight = i*32/length;
        if (weight!=alpha) {
            alpha = weight;
            PORTD=PORTB=color_map[color1*alpha + color2*(32-alpha) >> 5] | flag;
        }
    }
}

// Fast horizontal line supporting overdraw and interpolation
// Does not support masking
void Uno_3D::interpolateFastHLine(int16_t x, int16_t y, uint8_t length, uint8_t color1, uint8_t color2) {
    if (length<1) return;
    #ifdef DO_CLIP
        int16_t x2=x+length-1;
        if(length<=0||y<0||y>=_height||x>=_width||x2<0) return;
        if(x<0) {length+=x; x=0;}
        if(x2>=_width) {x2=_width-1; length=x2-x+1;}
    #endif    
    SET_Y_LOCATION(y);
    if (!do_overdraw) {
        SET_X_LOCATION(x);   
        interpolateFlood(y,0,length,length,color1,color2);
        return;
    }
    int in_segment=0;
    int start=x;
    int stop =x+length;
    int i=x;
    
    uint8_t line_flag = 0b1000*(y&1);
    uint8_t mask_test = mask_flag^line_flag;
    uint8_t background_mask = (background_color>>8) & 0b11110100;
    
    while (i<stop) {
        SET_X_LOCATION(i);
        START_READING();
        while (i<stop) {
            uint8_t read = PIND&0b11111100;
            uint8_t is_masked = (read&0b11110111)!=background_mask && (read&0b1000)==mask_test;
            PORTC=SEND_DATA;  
            PORTC=READY_READ;
            PORTC=SEND_DATA; 
            if (is_masked) {
                if (in_segment) {
                    STOP_READING();
                    SET_X_LOCATION(start);
                    interpolateFlood(y,start-x,i-x,length,color1,color2);
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
        interpolateFlood(y,start-x,i-x,length,color1,color2);
    }
}

// Shade a triangle with three colors
// Supports optional overdraw rendering
// Does not support masking -- masking is typically used to erases so
// will not be needed here.
void Uno_3D::shadeTriangle ( 
          int16_t x0, int16_t y0,
		  int16_t x1, int16_t y1,
		  int16_t x2, int16_t y2, 
          uint8_t color0, 
          uint8_t color1, 
          uint8_t color2) {
    int16_t a, b, y, last;
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); swapU8(color0,color1);}
    if (y1 > y2) { swap(y2, y1); swap(x2, x1); swapU8(color2,color1);}
    if (y0 > y1) { swap(y0, y1); swap(x0, x1); swapU8(color0,color1);}
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
    uint8_t midpoint = interpolate(color2,color0,(y1-y0)*32./(y2-y0));
    uint8_t colorX = midpoint;
    uint8_t colorY = color1;
    uint8_t colorZ = color0;
    // Skip the first line to avoid triangle overlap in meshes
    sa += dx01;
    sb += dx02;
    for(y=y0+1; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        float weight = ((y-y0)*32.)/len2;
        uint8_t colorA = interpolate(colorX,colorZ,weight);
        uint8_t colorB = interpolate(colorY,colorZ,weight);
        if(a > b) {
            swap(a,b);
            swap(colorA,colorB);
        }
        interpolateFastHLine(a, y, b-a, colorA, colorB);
    }
    uint8_t colorU = midpoint;
    uint8_t colorV = color1;
    uint8_t colorW = color2;
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for(; y<y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        float weight = 32.-((y-y1)*32.)/len3;
        uint8_t colorA = interpolate(colorU,colorW,weight);
        uint8_t colorB = interpolate(colorV,colorW,weight);
        if(a > b) {
            swap(a,b);
            swap(colorA,colorB);
        }
        interpolateFastHLine(a, y, b-a, colorA, colorB);
    }
}




