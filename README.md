# 3D rendering library for Arduino Uno and TFT display

This is a derivative of the Adafruit graphics libraries. I have copied the original README files below. To achieve the requisite performance optimizations, this library is no longer compatible with multiple LCD displays. This project is intended only for use of the 9341 TFT LCD display with the Arduino Uno.

This project adds the following:

1. C++ libraries for rendering 3D graphics
2. Modifications to the Uno display drivers needed for 3D rendering
3. Significant performance improvements in rendering speed for the Arduino Uno
4. Some models / demonstration sketches for 3D rendering
5. Rudimentary Leonardo support (small program memory size negatively impacts performance)

Unless otherwise specified or in conflict with the above BSD license from
Adafruit (below), all media, text, and documentation are licensed under the 
Creative Commons Attribution Share Alike 4.0 (CC BY-SA 4.0) license.
All source code added to this project (i.e. not copyright of Adafruit)
is licensed under the Gnu Public License 3.0 (GPLv3). Please note that
the CC BY-SA 4.0 license is one-way compatible with the GPLv3 license.

<img src="ihttps://raw.githubusercontent.com/michaelerule/Uno9341TFT/master/Montage.png" width="100%">

### See also

[A short video introduction](https://vimeo.com/150386845)

Full details can be found in [this writeup](http://crawlingrobotfortress.blogspot.de/2015/12/better-3d-graphics-engine-on-arduino.html)

[Hackaday post](https://hackaday.com/2016/01/02/better-3d-graphics-on-the-arduino/)

### Installing for Arduino

 - Clone this repository
 - [Follow these instructions](https://www.arduino.cc/en/guide/libraries) to add the library to Arduino IDE
 - Load sketches in Examples folder onto Arduino UNO (Leonardo support is limited and buggy). It may work on other AtMega based Arduinos, but this has not been tested. 
 - The 9341 TFT LCD display might interfere with program uploading for some sketches; Try uploading programs without it attached. 

### Benchmarks 

Performance improvements (Rendering time is in μs):
  
Benchmark|Adafruit|9341-Uno Basic Draw|Speedup|9341-Uno Fast Draw|Speedup
-----------------|----------|-----------------|-----------------|-----------------|-----------------
Fill|1321696|117784|11.22|97404|13.57
Text|117828|54516|2.16|53168|2.22
Lines|605668|149116|4.06|56252|10.77
Horiz/Vert Lines|125672|12084|10.40|9120|13.78
Draw Rectangles|83380|9168|9.09|6792|12.28
Fill Rectangles|3064596|244692|12.52|206644|14.83
Fill Circles|637896|134216|4.75|115120|5.54
Draw Circles|307316|83028|3.70|92904|3.31
Draw Triangles|192564|45608|4.22|15540|12.39
Fill Triangles|1360872|433896|3.14|331112|4.11
Draw Rounded|145428|36440|3.99|38116|3.82
Fill Rounded|3405668|519204|6.56|504784|6.75

Basic Drawing benchmarks: `./exammples/self_contained_Uno9341TFT_graphics_test`

Fast Drawing benchmarks: `./exammples/self_contained_Uno9341TFT_graphics_test_fast_commands`

---------------------------------------------------------------------------


# Adafruit GFX Library

This is the core graphics library for all our displays, providing a common set of graphics primitives (points, lines, circles, etc.). It needs to be paired with a hardware-specific library for each display device we carry (to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information.
All text above must be included in any redistribution.

---------------------------------------------------------------------------

## Optimisations

#### Color filling: 

The 9351 TFT LCD displays use an 8-bit data bus to write/read color data and send configuration commands. 

The 9351 displays operate in 16-bit color, so the low/high bytes of each color are sent separately. This means that we need to constantly switch the Arduino pin states from the high and low byte of the color data when filling part of the screen. This is expensive. 

The Uno9341TFT library optimizes this by detecting when the high and low byte of the color are the same. It configures the output lines to this byte, and then strobes the data clock twice for each pixel to send. The optimized `flood` routine also unrolls the loop for testing how many pixels to send.

The subset of 16-bit colors with matching high and low bytes are caled "fast colors", and the pallet is less restrictive than one might expect. Six "fast" color maps for shading 3D surfaces are defined in [color_maps.h](https://github.com/michaelerule/Uno9341TFT/blob/master/color_maps.h)


#### `setAddrWindow`

In the Adafruit library, the macro `setAddrWindow` is used to specify a rectangular region of the screen to send data to. For example, you can fill a rectangle by setting `setAddrWindow` to its limits, and then piping the color data to this region using the `flood` routine. 

In the original Adafruit code, `setAddrWindow` was called twice for many drawing commands: first to set up the region for drawing, and then again at the end to restore the address rectangle to the full screen size.

The Uno9341TFT library optimizes this by 
1. Setting only those components of the address ranges that are absolutly required to complete the current drawing command, and 
2. failing to "clean up" the address rectangle after each drawing command. 
Where the Adafruit library called `setAddrWindow` you will instead see one of the macros `SET_XY_RANGE`, `SET_XY_LOCATION`, `SET_X_LOCATION`, `SET_Y_LOCATION`, `RESET_X_RANGE`, or `ZERO_XY`.

The Uno9341TFT library also assumes a 320x240 pixel display in portrait mode. So, it assumes that only a single byte is needed to specify the `x` location. This means that the drawing commands won't work correctly in landscape mode. It is easy to transpose `x` and `y` coordinates in user code, so this is no limitation. 


#### `FRAME_ID_BIT`

The Arduino doesn't have enough memory (or speed) to support double-buffering. To avoid flickering when rendering 2D or 3D animations, it is useful to draw the next frame before erasing the pixels from the previous frame. 

To do this, we store a single `FRAME_ID_BIT` in the color data. It flips between `0` and `1` on alternate frames. So, we can easily tell if a pixel matches the background color, or comes from the foreground of the current or previous frame. 

Let's say `FRAME_ID_BIT` was `0` on the previous frame. To render the next frame, we first render the model using colors with the `FRAME_ID_BIT` set to `1`. We then "erase" the previous frame by "redrawing" it, but instead of sending color data, we read the color data back from the screen and check the `FRAME_ID_BIT`. Every color with `FRAME_ID_BIT=0` is from the previous frame, and can be erased by over-painting it with the background color. 

The `FRAME_ID_BIT` is part of the color data, so it changes the visible colors on the screen. To reduce visible disruption, the `FRAME_ID_BIT` is alternated between `0` and `1` on alternate scanlines. `FRAME_ID_BIT` is also used to handle the occlusion problem in 3D rendering.


### 3D library

3D models are stored in program memory as a list of vertices and edges and/or triangles.

- A model is rendered by applying a 3×3 rotation matrix to the vertex set. 
- We use [Back-face culling](https://en.wikipedia.org/wiki/Back-face_culling) to draw only half the triangles in the model. 
- For surface rendering, triangles must be sorted by their Z coordinate.
  - Bubble sort is the best option, since the triangle list will remain mostly sorted and it operates in place.
- All triangles are drawn. The curent `FRAME_ID_BIT` is set so that we can distinguish past from current pixels.
  -  This is also used to avoid over-painting triangles that have already been rendered in the current frame, handling the occlusion problem. 
- Any pixels from the foreground of the previous frame that are still visible are over-painted with the background color. This is checked by reading the `FRAME_ID_BIT` back from the display.
  - For wireframe rendering, this is optimized by re-rendering the wireframe of the previous frame, so that we check only those pixels that might have been drawn.
  - For surface rendering, its faster to simply iterate over the bounding box of the previous frame. 

We implement flat shading by checking the per-triangle face normals, and using 16-color maps [color_maps.h](https://github.com/michaelerule/Uno9341TFT/blob/master/color_maps.h) that can be written quickly to the display. The Arduino is too slow for Phong shading, but Gouraud shading is possible. 



