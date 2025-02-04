#include "DryingController.hpp"
#include <Arduino.h> // Include Arduino library for ESP32
#include "constants.hpp"

// Constructor
DryingController::DryingController(float initialRHset, float initialHysteresis, int pin)
    : RHset(initialRHset), hysteresis(initialHysteresis), dryerPin(pin), dryerOn(false) {
    initGpioPin(dryerPin); // Initialize the GPIO pin
}

// Function to initialize the GPIO pin
void DryingController::initGpioPin(int pin) {
    pinMode(pin, OUTPUT); // Set the pin as output
    digitalWrite(pin, RELAY_OFF); // Ensure the dryer is off initially
}

// Function to set the GPIO pin state
void DryingController::setGpioState(bool state) {
    digitalWrite(dryerPin, state ? RELAY_ON : RELAY_OFF); // Set the pin state
}

// Function to control the dryer
bool DryingController::controlDryer(float RH) {
    if ((RH > RHset /*+ hysteresis*/) && (dryerActive == 1)) {
        dryerOn = true;
    } else if ((RH < (RHset - hysteresis)) || (dryerActive == 0)) {
        dryerOn = false;
    }
    setGpioState(dryerOn); // Set the GPIO state according to the dryer state
    return dryerOn;
}

// Function to get the dryer status
bool DryingController::isDryerOn() const {
    return dryerOn;
}

// Function to set a new RHset value
void DryingController::setRHset(float newRHset) {
    RHset = newRHset;
}

// Function to set a new hysteresis value
void DryingController::setHysteresis(float newHysteresis) {
    hysteresis = newHysteresis;
}
// function to enable/disable heater function 
int DryingController::setDryerActiveStatus(int status) {
    dryerActive = status;
    return status;
}

// function to enable/disable heater function 
int DryingController::getDryerActiveStatus() {
    return dryerActive;
}

// Function to get present hysteresis value
float DryingController::getHysteresis() {
    return hysteresis;
}

// Function to get present temp setting value
float DryingController::getRHSetting() {
    return RHset;
}