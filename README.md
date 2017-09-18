# 3D rendering library for Arduino Uno and TFT display

This is a derivative of the Adafruit graphics libraries. I have copied the 
original README files below. Please note that in order to achieve the
requisite performance optimizations, this library is no longer compatible
with multiple LCD displays. This project is intended only for use of the
9341 TFT LCD display with the Arduino Uno.

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

[Writeup](http://crawlingrobotfortress.blogspot.de/2015/12/better-3d-graphics-engine-on-arduino.html)

[Hackaday post](https://hackaday.com/2016/01/02/better-3d-graphics-on-the-arduino/)

---------------------------------------------------------------------------


# Adafruit GFX Library

This is the core graphics library for all our displays, providing a common set of graphics primitives (points, lines, circles, etc.). It needs to be paired with a hardware-specific library for each display device we carry (to handle the lower-level functions).

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.
BSD license, check license.txt for more information.
All text above must be included in any redistribution.





