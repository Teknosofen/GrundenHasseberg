#ifndef CONSTANTS_H
#define CONSTANTS_H

// I2C
#define SDA_PIN 43
#define SCL_PIN 44

// Graphix CFG
#define LCD_WIDTH  536
#define LCD_HEIGHT 240

// clockpos
#define CLOCK_SIZE 60
#define CLOCK_XPOS 474
#define CLOCK_YPOS 178

// Controller setup
// ----------------
#define INITIAL_SETTEMP 5.0
#define INITIAL_SETTEMPHYSTERESIS 1.0

#define INITIAL_SETRH 70.0
#define INITIAL_SETRHHYSTERESIS 2.0

#define HEATER_RELAY   1
#define DRYER_RELAY    2
#define HOTWATER_RELAY 3
#define SPARE_RELAY    10

#define RELAY_ON LOW    // Relays are active low
#define RELAY_OFF HIGH  // So we use HIGH to turn them off

// Create an instance of the MQTTHandler class
#define myMqttServer "192.168.1.147"                        // My MQTT broker IP
// const char* myMqttServer = "192.168.1.147";              // My MQTT broker IP
#define myMqttPort   1883                                   // Default MQTT port
// const int   myMqttPort   = 1883;                         // Default MQTT port
// String mqttUser = "Teknosofen";
// String mqttPW = "HassebergsGrund2025";

// WIFI CFG
// const int WiFiconfigPin = 15;
#define WiFiconfigPin 15

// Temp and humidity setup
#define SEA_LEVEL_PRESSURE 1013.0f

// EEPROM addresses

// const char* apSSID = "Grunden";
// const char* apPassword = "";
// #define ssidAddress 0
// #define passwordAddress 32
// const char* apDeviceName = "HassebergsGrund";

const int ssidAddress = 0;
const int passwordAddress = 32;

const int mqttIPAddress = 64;
const int mqttPortAddress = 96;

const int signatureAddress = 128;
const int settingsStartAddress = signatureAddress + 4; // Start address for actual settings
const uint32_t EEPROM_SIGNATURE = 0xDEADBEEF;

const int tempAddress = settingsStartAddress;
const int rhAddress = settingsStartAddress + sizeof(float);
const int tempHystAddress = settingsStartAddress + 2 * sizeof(float);
const int rhHystAddress = settingsStartAddress + 3 * sizeof(float);
const int heaterStatusAddress = settingsStartAddress + 4 * sizeof(float);
const int dryerStatusAddress = settingsStartAddress + 4 * sizeof(float) + sizeof(int);

#endif // CONSTANTS_H