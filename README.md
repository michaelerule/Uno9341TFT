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

(Rendering time in μs)
  
Benchmark        |9341-Uno  | Adafruit | Speedup 
-----------------|----------|----------|--------
Fill             | 117784   | 1321696  |  11.22  
Text             | 54516    | 117828   |   2.16  
Lines            | 149116   | 605668   |   4.06  
Horiz/Vert Lines | 12084    | 125672   |  10.40  
Draw Rectangles  | 9168     | 83380    |   9.09  
Fill Rectangles  | 244692   | 3064596  |  12.52  
Draw Circles     | 134216   | 637896   |   4.75  
Fill Circles     | 83028    | 307316   |   3.70  
Draw Triangles   | 45608    | 192564   |   4.22  
Fill Triangles   | 433896   | 1360872  |   3.14  
Draw Rounded     | 36440    | 145428   |   3.99  
Fill Rounded     | 519204   | 3405668  |   6.56  


---------------------------------------------------------------------------


# Adafruit GFX Library

This is the core graphics library for all our displays, providing a common set of graphics primitives (points, lines, circles, etc.). It needs to be paired with a hardware-specific library for each display device we carry (to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information.
All text above must be included in any redistribution.





