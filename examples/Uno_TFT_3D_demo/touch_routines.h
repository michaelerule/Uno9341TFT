// CONFIGURE TOUCH SCREEN STUFF
#include <TouchScreen.h>

// control lines for touch screen
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!secret
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// resistance values for touchscreen corners
#define TS_MINX 200
#define TS_MINY 200
#define TS_MAXX 920
#define TS_MAXY 900

// resistance values for touchscreen pressure
#define MINPRESSURE 2
#define MAXPRESSURE 1000

// debouncing coefficient for buttons
#define TOUCH_DEBOUNCE 100
#define MODEL_DEBOUNCE 25

uint16_t  touch_down = 0;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

uint8_t get_touch(int16_t *x, int16_t *y, int16_t *z) {
    PORTC=0b11111111;
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
        *x = map(p.x, TS_MINX, TS_MAXX, tft.width() , 0);
        *y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
        *z = p.z;
        return 1;
    }
    return 0;
}

// damping coefficients for rotation
#define MAXD 1.5
#define MIND 0.01
#define ALPHA 0.7
#define BETA (1.0-ALPHA)
// Drag time-out: initial position reset once this counts to zero
uint16_t model_drag = 0;
TSPoint p; // current touch location if touch down, else unchanged.
TSPoint q; // most recent touch location
// Using chained exponential damping
float ddx=0,ddy=0,DX=0,DY=0;
void damped_rotation() {
    float dx, dy;
    dx = (p.x-q.x)*0.02;
    dy = (p.y-q.y)*0.02;
    if (dx<-MAXD) dx=-MAXD;
    if (dy<-MAXD) dy=-MAXD;
    if (dx> MAXD) dx=MAXD;
    if (dy> MAXD) dy=MAXD;
    q.x=p.x;
    q.y=p.y;
    // add damping
    ddx  = ddx *ALPHA+BETA*dx;
    ddy  = ddy *ALPHA+BETA*dy;
    DX   = DX  *ALPHA+BETA*ddx;
    DY   = DY  *ALPHA+BETA*ddy;
}

void poll_touch() {
    if (get_touch(&touch_x,&touch_y,&touch_z)) {
        if (color_choosing_mode) {
            color_chooser_touch();
            touch_down=TOUCH_DEBOUNCE;
        } else {
            button_touch(); 
            if (touch_y<=BUTTON_SIZE || touch_y>=320-BUTTON_SIZE) {
                // touch is outside rendering area. might be a button press
                touch_down=TOUCH_DEBOUNCE;
            } else {
                // touch is within rendering area. update damped rotation model.
                p.x = touch_x;
                p.y = touch_y;
                if (model_drag==0) {
                    q.x=p.x;
                    q.y=p.y;
                    ddx = ddy = DX = DY = 0;
                }
                model_drag=MODEL_DEBOUNCE;
            }
        }
    } else {
        // possibly send a touch-up event for the buttons 
        if (touch_down>0) {
            touch_down --;
            if (touch_down==0) {
                if (color_choosing_mode) color_chooser_event();    
                else button_event();
            }
        }
        if (model_drag>0) model_drag --;
    }
    damped_rotation();
}
