#ifndef ANALOGCLOCK_HPP
#define ANALOGCLOCK_HPP

#include <TFT_eSPI.h>
#include <time.h>

class AnalogClock {
public:
    AnalogClock(TFT_eSprite *sprite, int x, int y, int size);
    void begin();
    void update();
    void setRTCTime();

private:
    TFT_eSprite *sprite;
    int centerX;
    int centerY;
    int clockSize;
    void drawClockFace();
    void drawHand(float angle, int length, int color);
    float radiansFromTime(int value, int max);    
};

#endif // ANALOGCLOCK_HPP
