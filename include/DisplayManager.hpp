#ifndef DISPLAY_MANAGER_HPP
#define DISPLAY_MANAGER_HPP

#include "rm67162.h"
#include <TFT_eSPI.h>
#include <DFRobot_BME280.h>
#include "AnalogClock.hpp"
#include "WiFiConfig.hpp"
#include "MQTTHandler.hpp"


class DisplayManager {
public:
    DisplayManager(TFT_eSPI* tft, TFT_eSprite* spr, AnalogClock* clock);
    void begin();
    void updateWiFiStatus(int wifiStatus, WiFiConfig& wifiConfig);
    void updateMQTTStatus(MQTTHandler& mqttHandler);
    void updateActionInfo();
    void updateSensorData(float temperature, float pressure, float humidity);
    void updateControllerStatus(bool heaterOn, bool dryerOn);
    void renderHeader();
    void renderWiFi();
    void renderMQTTStatus();
    void renderActionInfo();
    void render();

private:

    // Constants for display layout
    static const int WIDTH = 536;
    static const int HEIGHT = 240;

    // static const int CLOCK_SIZE = 60;
    // static const int CLOCK_XPOS = 274;
    // static const int CLOCK_YPOS = 178;

    static const int firstColStart = 5;
    static const int secondColStart = 250;
    static const int firstLine = 5;
    static const int secondLine = 35;
    static const int thirdLine = 60;
    static const int fourthLine = 90;
    static const int fifthLine = 120;
    static const int sixthLine = 150;
    static const int seventhLine = 180;



    TFT_eSPI* tft;
    TFT_eSprite* spr;
    AnalogClock* clock;

    float temperature;
    float pressure;
    float humidity;

    bool heaterOn;
    bool dryerOn;

    int wifiStatus;
    String wifiDetailsRow1;
    String wifiDetailsRow2;

    String mqttDetailsRow1;
    String mqttDetailsRow2;

    String actionDetailsRow1;
    String actionDetailsRow2;
        
    void renderClock();
    // void renderHeader();
    // void renderWiFi();
    void renderSensorData();
    void renderControllerStatus();
    // void renderMQTTStatus();
};

#endif