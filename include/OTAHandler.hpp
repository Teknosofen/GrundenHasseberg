#ifndef OTAHANDLER_HPP
#define OTAHANDLER_HPP

#include <ArduinoOTA.h>

class OTAHandler {
public:
    void begin();
    void handle();
};

#endif // OTAHANDLER_HPP
