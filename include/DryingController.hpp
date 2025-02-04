#ifndef DRYINGCONTROLLER_HPP
#define DRYINGCONTROLLER_HPP

class DryingController {
private:
    float RHset;
    float hysteresis;
    int dryerPin;
    bool dryerOn;
    bool dryerActive = 1; // 2B used to disable the function in case of cf sensor failure

    // Function to initialize the GPIO pin
    void initGpioPin(int pin);

    // Function to set the GPIO pin state
    void setGpioState(bool state);

public:
    // Constructor
    DryingController(float initialRHset, float initialHysteresis, int pin);

    // Function to control the dryer
    bool controlDryer(float RH);

    // Function to get the dryer status
    bool isDryerOn() const;

    // Function to set a new RHset value
    void setRHset(float newRHset);

    // Function to set a new hysteresis value
    void setHysteresis(float newHysteresis);
        
    // Function to get hysteresi setting
    float getHysteresis(void);

       // Function to set a new hysteresis value
    float getRHSetting(void);

    int setDryerActiveStatus(int status);

    int getDryerActiveStatus(void);
    
};

#endif // DRYINGCONTROLLER_HPP



/* use:

#include <Arduino.h>
#include "DryingController.hpp"

DryingController dc(50.0, 5.0, 18); // Initialize with RHset, hysteresis, and GPIO pin (e.g., GPIO18)

void setup() {
    Serial.begin(115200); // Initialize serial communication for debugging
}

void loop() {
    float currentRH = 55.0; // Example relative humidity value

    if (dc.controlDryer(currentRH)) {
        Serial.println("Dryer is ON");
    } else {
        Serial.println("Dryer is OFF");
    }

    // Adjust the RHset and hysteresis values for demonstration purposes
    dc.setRHset(60.0);
    dc.setHysteresis(10.0);
    currentRH = 65.0; // New example relative humidity value

    if (dc.controlDryer(currentRH)) {
        Serial.println("Dryer is ON");
    } else {
        Serial.println("Dryer is OFF");
    }

    delay(5000); // Wait for 5 seconds before repeating the loop
}



*/