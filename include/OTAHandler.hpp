#ifndef OTAHANDLER_HPP
#define OTAHANDLER_HPP

#include <ArduinoOTA.h>

class OTAHandler {
public:
    void begin();
    void handle();
    void restart();  // Re-initialise OTA service after a WiFi reconnection
};

#endif // OTAHANDLER_HPP
