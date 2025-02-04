#include "HeaterController.hpp"
#include <Arduino.h> // Include Arduino library for ESP32
#include "constants.hpp"

// Constructor
HeaterController::HeaterController(float initialTset, float initialHysteresis, int pin )
    : Tset(initialTset), hysteresis(initialHysteresis), heaterPin(pin), heaterOn(false)  {
    initGpioPin(heaterPin); // Initialize the GPIO pin
}

// Function to initialize the GPIO pin
void HeaterController::initGpioPin(int pin) {
    pinMode(pin, OUTPUT); // Set the pin as output
    digitalWrite(pin, LOW); // Ensure the heater is off initially
}

// Function to set the GPIO pin state
void HeaterController::setGpioState(bool state) {
    digitalWrite(heaterPin, state ? RELAY_ON : RELAY_OFF); // Set the pin state
}

// Function to control the heater
bool HeaterController::controlHeater(float T) {
    if ((T < Tset /*- hysteresis*/) && (heaterActive == 1)) {
        heaterOn = true;
    } else if ((T > (Tset + hysteresis)) || (heaterActive == 0)){
        heaterOn = false;
    }
    setGpioState(heaterOn); // Set the GPIO state according to the heater state
    return heaterOn;
}

// Function to get the heater status
bool HeaterController::isHeaterOn() const {
    return heaterOn;
}

// Function to set a new Tset value
void HeaterController::setTset(float newTset) {
    Tset = newTset;
}
// function to enable/disable heater function 
int HeaterController::setHeaterActiveStatus(int status) {
    heaterActive = status;
    return status;
}
// function get heater enable/disablefunction
int HeaterController::getHeaterActiveStatus() {
    return heaterActive;
}

// Function to set a new hysteresis value
void HeaterController::setHysteresis(float newHysteresis) {
    hysteresis = newHysteresis;
}

// Function to get present hysteresis value
float HeaterController::getHysteresis() {
    return hysteresis;
}

// Function to get present temp setting value
float HeaterController::getTempSetting() {
    return Tset;
}


