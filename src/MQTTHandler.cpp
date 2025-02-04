#include "MQTTHandler.hpp"
#include <WiFi.h>
#include <PubSubClient.h>

// Constructor to initialize the broker address and port
MQTTHandler::MQTTHandler(const char* broker, int port, HeaterController *heatCtrl, DryingController *rHCtrl)
    : mqttBroker(broker), mqttPort(port), client(espClient), heatCtrl(heatCtrl), rHCtrl(rHCtrl) {}

// Initialize the MQTT client
void MQTTHandler::begin() {

    // EEPROM.begin(512); // Initialize EEPROM with size 512 bytes
    readSettingsFromEEPROM(); // Read settings from EEPROM
    
    client.setServer(mqttBroker, mqttPort);
    // Define the callback function for incoming messages
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->messageReceived(topic, payload, length);
    });
}

// Main loop to handle MQTT communication
void MQTTHandler::loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop(); // Ensure that the client processes incoming messages
}

bool MQTTHandler::isConnected() {
    return client.connected();
}

// Function to reconnect to the MQTT broker if disconnected
void MQTTHandler::reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect with a client ID
        if (client.connect("Hasseberg", mqttUser.c_str(), mqttPW.c_str())) {
            Serial.println("connected");
            // Subscribe to the required topics
            client.subscribe(setTempTopic);
            client.subscribe(setRHTopic);
            client.subscribe(setTempHysteresisTopic);
            client.subscribe(setRHHysteresisTopic);
            client.subscribe(getStatusTopic);
            client.subscribe(getHelpTopic);
            client.subscribe(getSettings);
            client.subscribe(getHeaterStatus);
            client.subscribe(getDryerStatus);
            client.subscribe(setHeaterStatus);
            client.subscribe(setDryerStatus);

        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// Function to handle incoming MQTT messages
void MQTTHandler::messageReceived(char* topic, byte* payload, unsigned int length) {
    String myTopic = topic;         // used to save the topic message that somehow gets mangled below
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    // Construct the first acknowledgment message
    char ackMsg[100];
    snprintf(ackMsg, sizeof(ackMsg), "Received message on topic %s: %s", topic, message.c_str());
    publish(ackTopic, ackMsg);

    // Construct and publish a second acknowledgment with clear-text string descriptions
    String clearTextAck;
    if (String(myTopic) == setTempTopic) {
        clearTextAck = handleSetTemp(message);
    } else if (String(myTopic) == setRHTopic) {
        clearTextAck = handleSetRH(message);
    } else if (String(myTopic) == setTempHysteresisTopic) {
        clearTextAck = handleSetTempHysteresis(message);
    } else if (String(myTopic) == setRHHysteresisTopic) {
        clearTextAck = handleSetRHHysteresis(message);
    } else if (String(myTopic) == getStatusTopic) {
        clearTextAck = handleGetStatus(message);
    } else if (String(myTopic) == getHelpTopic) {
        clearTextAck = handleGetHelp(message);
    } else if (String(myTopic) == getSettings) {
        clearTextAck = handleGetSettings(message);
    } else if(String(myTopic) == getHeaterStatus) {
        clearTextAck = handleGetHeaterStatus(message);
    } else if(String(myTopic) == getDryerStatus) {
        clearTextAck = handleGetDryerStatus(message);
    } else if(String(myTopic) == setHeaterStatus) {
        clearTextAck = handleSetHeaterStatus(message);
    } else if(String(myTopic) == setDryerStatus) {
        clearTextAck = handleSetDryerStatus(message);
    }
    else {
        clearTextAck = "Unknown topic or unprocessed message.";
    }

    // Publish the clear-text acknowledgment if it was set
    if (!clearTextAck.isEmpty()) {
        publish(ackTopic, clearTextAck.c_str());
    }
}

String MQTTHandler::handleGetHeaterStatus(const String& message) {
    String heatStat = heatCtrl->isHeaterOn()? "Heater: ON" : "Heater: OFF";
    return heatStat;
}
String MQTTHandler::handleGetDryerStatus(const String& message) {
    String dryerStat = rHCtrl->isDryerOn()? "Dryer: ON " : "Dryer: OFF ";
    return dryerStat;
}

String MQTTHandler::handleSetHeaterStatus(const String& message) {
    int status = message.toInt();
    if(status >= 0 && status <= 1) {
        heatCtrl->setHeaterActiveStatus(status);
        writeSettingToEEPROM(heaterStatusAddress, status);
        return "Heater status updated to " + String(status);
    } else {
        return "Invalid heater status value received.";
    }
    // heatCtrl->setHeaterStatus(status);
    // return "Heater status updated to " + String(status);
}

String MQTTHandler::handleSetDryerStatus(const String& message) {
    int status = message.toInt();
    if (status >= 0 && status <= 1) {
        rHCtrl->setDryerActiveStatus(status);
        writeSettingToEEPROM(dryerStatusAddress, status);
        return "Dryer status updated to " + String(status);
    } else {
        return "Invalid dryer status value received.";
    }
    // rHCtrl->setHeaterStatus(status);
    // return "Dryer status updated to " + String(status);
}

// Function to handle "getHelp" topic logic
String MQTTHandler::handleGetHelp(const String& message) {
    String helpTopicAck = "Can sub: " + String(setTempTopic) + ", " + String(setRHTopic) + ", " 
                            + String(setTempHysteresisTopic) + ", " + String(setRHHysteresisTopic) + ", " 
                            + String(getStatusTopic) + ", " + String(ackTopic) + ", " + String(getSettings) + ". "
                            + String(getHeaterStatus) + ", " + String(getDryerStatus) + ", " 
                            + String(setHeaterStatus) + ", " + String(setDryerStatus); // + ". "
                            /*+ "Topics available for subscription: " */
                            // + String(tempTopic) + ", " + String(rhTopic) + ", " + String(pBaroTopic) + ", " 
                            // + String(heaterStatusTopic) + ", " + String(dehumidifierStatusTopic);
    publish(ackTopic, helpTopicAck.c_str());
    helpTopicAck = "can pub: " + String(tempTopic) + ", " + String(rhTopic) + ", " + String(pBaroTopic) + ", " 
                            + String(heaterStatusTopic) + ", " + String(dehumidifierStatusTopic);
    return helpTopicAck;
}

String MQTTHandler::handleGetSettings(const String& message) {
    String settingsAck = "Set: ";
    settingsAck += "T set: " + String(heatCtrl->getTempSetting()) + "°C, ";
    settingsAck += "T hyst: " + String(heatCtrl->getHysteresis()) + "°C, ";
    // settingsAck += "Haat active: " + String(heatCtrl->getHeaterActiveStatus()) + " ";
    String heatStat = heatCtrl->getHeaterActiveStatus()? "Enabled" : "Disabled";
    // settingsAck += "Heat active: " + ((heatCtrl->getHeaterActiveStatus() == 0)? "ON": "OFF"); // String(heatCtrl->getHeaterActiveStatus()) + " ";
    settingsAck += "Heat: " + heatStat + ", ";

    settingsAck += "RH set: " + String(rHCtrl->getRHSetting()) + "%, ";
    settingsAck += "RH hyst: " + String(rHCtrl->getHysteresis()) + "%.";
    String dryerStat = rHCtrl->getDryerActiveStatus()? "Enabled" : "Disabled";
    settingsAck += "Dryer: " + dryerStat; //String(rHCtrl->getDryerActiveStatus());


    // add statis on heat and dying as well
    return settingsAck;
}
// Function to handle "setTemp" topic logic
String MQTTHandler::handleSetTemp(const String& message) {
    float setTemp = message.toFloat();
    if (setTemp >= TEMP_MIN && setTemp <= TEMP_MAX) {
        // Set temperature logic here
        heatCtrl->setTset(setTemp);
        writeSettingToEEPROM(tempAddress, setTemp);
        return "Temperature setting updated to " + String(setTemp) + "°C.";
    } else {
        return "Invalid temperature value received.";
    }
}
// Function to handle "setTempHysteresis" topic logic
String MQTTHandler::handleSetTempHysteresis(const String& message) {
    float setTempHyst = message.toFloat();
    if (setTempHyst >= TEMP_HYSTERESIS_MIN && setTempHyst <= TEMP_HYSTERESIS_MAX) {
        heatCtrl->setHysteresis(setTempHyst);
        writeSettingToEEPROM(tempHystAddress, setTempHyst);
        return "Temperature  hysteresis updated to " + String(setTempHyst) + "%.";
    } else {
        return "Invalid Temperature  hysteresis value received.";
    }
}

// Function to handle "setRH" topic logic
String MQTTHandler::handleSetRH(const String& message) {
    float setRH = message.toFloat();
    if (setRH >= RH_MIN && setRH <= RH_MAX) {
        rHCtrl->setRHset(setRH);// Set RH logic here
        writeSettingToEEPROM(rhAddress, setRH);
        return "Relative humidity setting updated to " + String(setRH) + "%.";
    } else {
        return "Invalid relative humidity value received.";
    }
}

// Function to handle "setRHHysteresis" topic logic
String MQTTHandler::handleSetRHHysteresis(const String& message) {
    float setRHHyst = message.toFloat();
    if (setRHHyst >= RH_HYSTERESIS_MIN && setRHHyst <= RH_HYSTERESIS_MAX) {
        rHCtrl->setHysteresis(setRHHyst); // Set RH logic here
        writeSettingToEEPROM(rhHystAddress, setRHHyst);
        return "Relative humidity hysteresis updated to " + String(setRHHyst) + "%.";
    } else {
        return "Invalid relative humidity hysteresis value received.";
    }
}

// Function to handle "setRHHysteresis" topic logic
String MQTTHandler::handleGetStatus(const String& message) {
    String heatStat = heatCtrl->isHeaterOn()? "Heater: ON" : "Heater: OFF";
    String dryerStat = rHCtrl->isDryerOn()? "Dryer: ON " : "Dryer: OFF ";
    
    return "Status: " + heatStat + ", " + dryerStat;
}

// Function to publish messages
void MQTTHandler::publish(const char* topic, const char* message) {
    if (client.connected()) {
        client.publish(topic, message);
        Serial.printf("Published to %s: %s\n", topic, message);
    } else {
        Serial.println("Not connected to MQTT broker, cannot publish.");
    }
}

void MQTTHandler::readSettingsFromEEPROM() {
    uint32_t signature = EEPROM.readUInt(signatureAddress);
    if (signature == EEPROM_SIGNATURE) {
        float temp = EEPROM.readFloat(tempAddress);
        float rh = EEPROM.readFloat(rhAddress);
        float tempHyst = EEPROM.readFloat(tempHystAddress);
        float rhHyst = EEPROM.readFloat(rhHystAddress);
        int heaterStatus = EEPROM.readInt(heaterStatusAddress);
        int dryerStatus = EEPROM.readInt(dryerStatusAddress);
// Debug
Serial.printf("tempSetting: %f\n", temp);
Serial.printf("RHSetting: %f\n", rh);
Serial.printf("tempHyst: %f\n", tempHyst);
Serial.printf("RH Hyst: %f\n", rhHyst);
Serial.printf("Heater status: %d\n", heaterStatus);
Serial.printf("Dryer status: %d\n", dryerStatus);

        if (!isnan(temp)) {
            heatCtrl->setTset(temp);
        }
        if (!isnan(rh)) {
            rHCtrl->setRHset(rh);
        }
        if (!isnan(tempHyst)) {
            heatCtrl->setHysteresis(tempHyst);
        }
        if (!isnan(rhHyst)) {
            rHCtrl->setHysteresis(rhHyst);
        }
        if (!isnan(heaterStatus)) {
            heatCtrl->setHeaterActiveStatus(heaterStatus);
        }
        if (!isnan(dryerStatus)) {
            rHCtrl->setDryerActiveStatus(dryerStatus);
        }
    } else {
        Serial.println("No valid data in EEPROM.");
    }
}

void MQTTHandler::writeSettingToEEPROM(int address, float value) {
    EEPROM.writeFloat(address, value);
    EEPROM.commit();
    Serial.printf("fkoat Setting written to EEPROM: %f\n", value);
    EEPROM.writeUInt(signatureAddress, EEPROM_SIGNATURE);       // Write signature to indicate valid data
    EEPROM.commit();
}

void MQTTHandler::writeSettingToEEPROM(int address, int value) {
    EEPROM.writeInt(address, value);
    EEPROM.commit();
    Serial.printf("Int Setting written to EEPROM: %f\n", value);
    EEPROM.writeUInt(signatureAddress, EEPROM_SIGNATURE);       // Write signature to indicate valid data
    EEPROM.commit();
}

void MQTTHandler::checkAndInitializeEEPROM() {
    uint32_t signature = EEPROM.readUInt(signatureAddress);
    if (signature != EEPROM_SIGNATURE) {
        // EEPROM is uninitialized, store default values
        EEPROM.writeUInt(signatureAddress, EEPROM_SIGNATURE);
        EEPROM.writeFloat(tempAddress, INITIAL_SETTEMP);
        EEPROM.writeFloat(rhAddress, INITIAL_SETRH);
        EEPROM.writeFloat(tempHystAddress, INITIAL_SETTEMPHYSTERESIS);
        EEPROM.writeFloat(rhHystAddress, INITIAL_SETRHHYSTERESIS);
        EEPROM.writeInt(heaterStatusAddress, 1);    // i.e. active
        EEPROM.writeInt(dryerStatusAddress, 1);     // i.e. active

        EEPROM.commit();
        Serial.println("EEPROM initialized with default values.");
    } else {
        Serial.println("EEPROM contains valid data.");
    }
}

String MQTTHandler::getMQTTIP() {
    return mqttBroker;
}

String MQTTHandler::getMQTTPort() {
    return String(mqttPort);
}