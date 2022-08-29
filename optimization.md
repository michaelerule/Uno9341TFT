## Optimisations

#### Color filling: 

The 9351 TFT LCD displays use an 8-bit data bus for bidirectional communiction. It can be used to write and read color data, and send configuration commands. 

The 9351 displays operate in 16-bit color, so the low/high bytes of each color are sent separately. This means that we need to constantly switch the Arduino pin states from the high and low byte of the color data when filling part of the screen. This is expensive. 

The Uno9341TFT library optimizes this by detecting when the high and low byte of the color are the same. It configures the output lines to this byte, and then strobes the data clock twice for each pixel to send. The optimized `flood` routine also unrolls the loop for testing how many pixels to send.

The subset of 16-bit colors with matching high and low bytes are caled "fast colors", and the pallet is less restrictive than one might expect. Six "fast" color maps for shading 3D surfaces are defined in [color_maps.h](https://github.com/michaelerule/Uno9341TFT/blob/master/color_maps.h)


#### `setAddrWindow`

In the Adafruit library, the macro `setAddrWindow` is used to specify a rectangular region of the screen to send data to. For example, you can fill a rectangle by setting `setAddrWindow` to its limits, and then piping the color data to this region using the `flood` routine. 

In the original Adafruit code, `setAddrWindow` was called twice for many drawing commands: first to set up the region for drawing, and then again at the end to restore the address rectangle to the full screen size.

The Uno9341TFT library optimizes this by (1) Setting only those components of the address ranges that are absolutly required to complete the current drawing command, and (2) failing to "clean up" the address rectangle after each drawing command. Where the Adafruit library called `setAddrWindow` you will instead see one of the macros `SET_XY_RANGE`, `SET_XY_LOCATION`, `SET_X_LOCATION`, `SET_Y_LOCATION`, `RESET_X_RANGE`, or `ZERO_XY`.

The Uno9341TFT library also assumes a 320x240 pixel display in portrait mode. So, it assumes that only a single byte is needed to specify the `x` location. This means that the drawing commands won't work correctly in landscape mode. But, it is easy to transpost `x` and `y` coordinates in user code, so this is no limitation. 


#### `FRAME_ID_BIT`

The Arduino doesn't have enough memory (or speed) to support double-buffering. To avoid flickering when rendering 2D or 3D animations, it is useful to draw the next frame before erasing the pixels from the previous frame. 

To do this, we store a single `FRAME_ID_BIT` in the color data. It flips between `0` and `1` on alternate frames. So, we can easily tell if a pixel matches the background color, or comes from the foreground of the current or previous frame. 

Let's say `FRAME_ID_BIT` was `0` on the previous frame. To render the next frame, we first render the model using colors with the `FRAME_ID_BIT` set to `1`. We then "erase" the previous frame by "redrawing" it, but instead of sending color data, we read the color data back from the screen and check the `FRAME_ID_BIT`. Every color with `FRAME_ID_BIT=0` is from the previous frame, and can be erased by overdrawing it with the background color. 

The `FRAME_ID_BIT` is part of the color data, so it changes the visible colors on the screen. To reduce visible disruption, the `FRAME_ID_BIT` is alternated between `0` and `1` on alternate scanlines. 

`FRAME_ID_BIT` is also used as a sort of 1-bit z-buffer to handle the occlusion problem in 3D rendering.


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

We implement flat shading by checking the per-triangle face normals, and using 16-color maps [color_maps.h](https://github.com/michaelerule/Uno9341TFT/blob/master/color_maps.h) that can be written quickly to the display. The Arduino is not fast enough for Phone shading, but Gouraud shading is possible. 







