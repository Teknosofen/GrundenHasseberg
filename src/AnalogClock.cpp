#include "AnalogClock.hpp"

// Constructor
AnalogClock::AnalogClock(TFT_eSprite *sprite, int x, int y, int size)
    : sprite(sprite), centerX(x), centerY(y), clockSize(size) {}

// Initialize the clock
void AnalogClock::begin() {
    // Draw the clock face
    drawClockFace();
    setRTCTime();   // Set the RTC time with the compile time clock.setRTCTime
    update();
}

// Update the clock
void AnalogClock::update() {
    // Get the current time
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    // Calculate angles for the hands
    float hourAngle = radiansFromTime(timeinfo.tm_hour % 12, 12) + radiansFromTime(timeinfo.tm_min, 60) / 12;
    float minuteAngle = radiansFromTime(timeinfo.tm_min, 60);
    float secondAngle = radiansFromTime(timeinfo.tm_sec, 60);

    // Clear previous hands
    sprite->fillCircle(centerX, centerY, clockSize - 4, TFT_BLACK);

    // Draw clock face again to cover erased hands
    drawClockFace();

    // Draw the hands
    drawHand(hourAngle, clockSize * 0.5, TFT_WHITE);   // Hour hand
    drawHand(minuteAngle, clockSize * 0.75, TFT_WHITE); // Minute hand
    drawHand(secondAngle, clockSize * 0.9, TFT_RED);   // Second hand
}
// Set the RTC time with the compile time 
void AnalogClock::setRTCTime() { 
    struct tm tm; // Build date and time in "YYYY/MM/DD HH:MM:SS" format 
    const char compileTime[] = __DATE__ " " __TIME__; 
    strptime(compileTime, "%b %d %Y %H:%M:%S", &tm); 
    time_t t = mktime(&tm); 
    struct timeval now = { .tv_sec = t }; 
    settimeofday(&now, NULL); 
    Serial.println("RTC time set to compile time: " + String(compileTime));
}


// Draw the clock face
void AnalogClock::drawClockFace() {
    // Draw the outer circle
    sprite->drawCircle(centerX, centerY, clockSize, TFT_WHITE);
    // Draw hour markers
    for (int i = 0; i < 12; i++) {
        float angle = radiansFromTime(i, 12);
        int x1 = centerX + (clockSize - 8) * cos(angle);
        int y1 = centerY + (clockSize - 8) * sin(angle);
        int x2 = centerX + clockSize * cos(angle);
        int y2 = centerY + clockSize * sin(angle);
        sprite->drawLine(x1, y1, x2, y2, TFT_WHITE);
    }
}

// Draw a hand
void AnalogClock::drawHand(float angle, int length, int color) {
    int x = centerX + length * cos(angle);
    int y = centerY + length * sin(angle);
    sprite->drawLine(centerX, centerY, x, y, color);
}

// Convert time value to radians
float AnalogClock::radiansFromTime(int value, int max) {
    // return (PI / 2 - 2 * PI * value / max);
    // Flip the angle calculation to correct the hand orientation 
    return (3 * PI / 2 + 2 * PI * value / max);
}
