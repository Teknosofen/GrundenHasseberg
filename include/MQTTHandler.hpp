#ifndef MQTTHandler_hpp
#define MQTTHandler_hpp

#include <WiFi.h>
#include <PubSubClient.h>
#include "constants.hpp"
#include "DryingController.hpp"
#include "HeaterController.hpp"
#include <EEPROM.h>

// MQTTHandler class definition
class MQTTHandler {
public:
    // Constructor to initialize the broker address and port
    MQTTHandler(const char* broker, int port, HeaterController *heatCtrl, DryingController  *rHCtrl);

    // Initialize the MQTT client
    void begin();

    // Main loop to handle MQTT communication
    void loop();

    // Function to publish a message to a topic
    void publish(const char* topic, const char* message);

    // Function to handle incoming MQTT messages
    void messageReceived(char* topic, byte* payload, unsigned int length);

    String getMQTTIP(); 

    String getMQTTPort();

    bool isConnected(void);

    // Function to check and initialize EEPROM
    void checkAndInitializeEEPROM();

    // MQTT topics for publishing data
    const char* ackTopic = "Grund/ack";
    const char* tempTopic = "Grund/temp";
    const char* rhTopic = "Grund/RH";
    const char* pBaroTopic = "Grund/pbaro";
    const char* heaterStatusTopic = "Grund/heatStatus";
    const char* dehumidifierStatusTopic = "Grund/dehumidStatus";

    // // EEPROM addresses
    // const int signatureAddress = 32;
    // const int settingsStartAddress = 4; // Start address for actual settings
    // const uint32_t EEPROM_SIGNATURE = 0xDEADBEEF;

    // const int tempAddress = settingsStartAddress;
    // const int rhAddress = settingsStartAddress + sizeof(float);
    // const int tempHystAddress = settingsStartAddress + 2 * sizeof(float);
    // const int rhHystAddress = settingsStartAddress + 3 * sizeof(float);
    // const int heaterStatusAddress = settingsStartAddress + 4 * sizeof(float);
    // const int dryerStatusAddress = settingsStartAddress + 4 * sizeof(float) + sizeof(int);

private:

	// Range validation limits
	#define TEMP_MIN 1.0
	#define TEMP_MAX 25.0
	#define RH_MIN 0.0
	#define RH_MAX 100.0
	#define TEMP_HYSTERESIS_MIN 0.1
	#define TEMP_HYSTERESIS_MAX 10.0
	#define RH_HYSTERESIS_MIN 1.0
	#define RH_HYSTERESIS_MAX 20.0

    String mqttUser = "Teknosofen";
    String mqttPW   = "HassebergsGrund2025";

    // MQTT broker address and port
    const char* mqttBroker;
    int mqttPort;

    HeaterController *heatCtrl;
    DryingController  *rHCtrl;

    // WiFiClient and PubSubClient objects
    WiFiClient espClient;
    PubSubClient client;

    // Function to reconnect to the MQTT broker if disconnected
    void reconnect();

    String handleSetTemp(const String& message);
    String handleSetTempHysteresis(const String& message);
    String handleSetRH(const String& message);
    String handleSetRHHysteresis(const String& message);
    String handleGetStatus(const String& message);
    String handleGetHelp(const String& message);
    String handleGetSettings(const String& message);
    String handleGetHeaterStatus(const String& message);
    String handleGetDryerStatus(const String& message);
    String handleSetHeaterStatus(const String& message);
    String handleSetDryerStatus(const String& message);
  
    void readSettingsFromEEPROM();
    void writeSettingToEEPROM(int address, float value);
    void writeSettingToEEPROM(int address, int value);


    // MQTT topics for incoming commands
    const char* setTempTopic = "Grund/setTemp";
    const char* setRHTopic = "Grund/setRH";
    const char* setTempHysteresisTopic = "Grund/setTempHyst";
    const char* setRHHysteresisTopic = "Grund/setRHHyst";
    const char* getStatusTopic = "Grund/getStatus";
    const char* getHelpTopic = "Grund/help";

    const char* getSettings = "Grund/getSet";
    const char* getHeaterStatus = "Grund/getHeatStatus";
    const char* getDryerStatus = "Grund/getDryStatus";
    const char* setHeaterStatus = "Grund/setHeatStatus";
    const char* setDryerStatus = "Grund/setDryStatus";


    // // MQTT topics for publishing data
    // const char* ackTopic = "Grund/ack";
    // const char* tempTopic = "Grund/temp";
    // const char* rhTopic = "Grund/RH";
    // const char* pBaroTopic = "Grund/pbaro";
    // const char* heaterStatusTopic = "Grund/heatStatus";
    // const char* dehumidifierStatusTopic = "Grund/dehumidStatus";
    
    };

#endif // MQTTHandler_hpp
