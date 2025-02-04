#ifndef WIFICONFIG_HPP
#define WIFICONFIG_HPP

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "constants.hpp"

class WiFiConfig {
public:
    WiFiConfig(int pin);
    int     begin();
    String  getMySelectedSSID();
    String  getMySelectedIP();
    String  getMyAPIP();
    String  getMyAPSSID();
    bool    getMyWiFiStatus();
    uint8_t getMyWiFiConStatus(); // 0 = no connection, 1 = AP, 2 = STA
    
private:
    int configPin;
    const char* apSSID = "Grunden";
    const char* apPassword = "";
    // #define ssidAddress 0
    // #define passwordAddress 32
    const char* apDeviceName = "HassebergsGrund";

    AsyncWebServer server;
    String ssidStr, passwordStr;
    char ssid[32] = "";
    char password[32] = "";

    uint8_t WiFiConStatus = 0;

    #define WL_NO_SHIELD        255   // for compatibility with WiFi Shield library
    #define WL_IDLE_STATUS      0
    #define WL_NO_SSID_AVAIL    1
    #define WL_SCAN_COMPLETED   2
    #define WL_CONNECTED        3
    #define WL_CONNECT_FAILED   4
    #define WL_CONNECTION_LOST  5
    #define WL_DISCONNECTED     6

    void setupAP();
    void connectToWiFi();
    void handleRoot(AsyncWebServerRequest* request);
    void handleSave(AsyncWebServerRequest* request);
    String scanNetworks(int maxMsPerChan = 100, int channel = 1); // Default values provided 
    String wl_status_to_string(int status);
    
};

#endif // WIFICONFIG_HPP


