#ifndef HEATERCONTROLLER_HPP
#define HEATERCONTROLLER_HPP

class HeaterController {
private:
    float Tset;
    float hysteresis;
    int   heaterPin;
    bool  heaterOn = false;
    int   heaterActive = 1; // 2B used to disable the function in case of cf sensor failure

    // Function to initialize the GPIO pin
    void initGpioPin(int pin);

    // Function to set the GPIO pin state
    void setGpioState(bool state);

public:
    // Constructor
    HeaterController(float initialTset, float initialHysteresis, int pin);

    // Function to control the heater
    bool controlHeater(float T);

    // Function to get the heater status
    bool isHeaterOn() const;

    // Function to set a new Tset value
    void setTset(float newTset);

    // Function to set a new hysteresis value
    void setHysteresis(float newHysteresis);

    // Function to get hysteresi setting
    float getHysteresis(void);

       // Function to set a new hysteresis value
    float getTempSetting(void);

    int setHeaterActiveStatus(int status);
    
    int getHeaterActiveStatus(void);

};

#endif // HEATERCONTROLLER_HPP