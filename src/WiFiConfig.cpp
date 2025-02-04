#include <Arduino.h>
#include "WiFiConfig.hpp"
#include <EEPROM.h>
#include <WiFi.h>
#include "constants.hpp"

// #include <WebServer.h>

// Constructor
WiFiConfig::WiFiConfig(int pin) : configPin(pin), server(80) {}

// Begin method implementation
int WiFiConfig::begin() {
    EEPROM.begin(512);

    ssidStr = EEPROM.readString(ssidAddress);
    passwordStr = EEPROM.readString(passwordAddress);

    Serial.print("from EEPROM: SSID: ");
    Serial.print(ssidStr);
    Serial.println(" pw: " + passwordStr);

    int wiFiStatus = 0;

    pinMode(configPin, INPUT);
    if ((digitalRead(configPin) == HIGH) || (ssidStr.length() == 0)) { // check if config switch is set or EEPROM empty
        Serial.println("Setting up AP");
        setupAP();
        wiFiStatus = 1;     // so the correct message can be displayed
    } else {
        Serial.println("Connecting to WiFi");
        connectToWiFi();
        wiFiStatus = 0;     // so the correct message can be displayed
    }
    return wiFiStatus;
}

// Other method implementations
void WiFiConfig::setupAP() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.softAP(apSSID, apPassword);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    WiFiConStatus = 1; // AP connected
    server.on("/", HTTP_GET, std::bind(&WiFiConfig::handleRoot, this, std::placeholders::_1));
    server.on("/save", HTTP_POST, std::bind(&WiFiConfig::handleSave, this, std::placeholders::_1));
    server.begin();
}

void WiFiConfig::connectToWiFi() {
    WiFi.disconnect();                                      // Reset WiFi settings
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.mode(WIFI_STA);

    ssidStr = EEPROM.readString(ssidAddress);               // get the stored SSID and PW
    passwordStr = EEPROM.readString(passwordAddress);

    const char* ssid = ssidStr.c_str();                     // just a try to see if it will connect...
    const char* password = passwordStr.c_str(); 
    WiFi.setHostname(apDeviceName);
    WiFi.begin(ssid, password);
    // WiFi.begin(ssidStr, passwordStr);

    int retryCount = 0; 
    while (WiFi.status() != WL_CONNECTED && retryCount < 20) { 
        delay(500); 
        Serial.print("."); 
        retryCount++; 
    } 

    if (WiFi.status() == WL_CONNECTED) { 
        Serial.println("Connected to WiFi"); 
        Serial.print("IP Address: "); 
        Serial.println(WiFi.localIP()); 
        WiFiConStatus = 2; // STA connected
    }
    else { 
        Serial.println("Failed to connect to WiFi"); 
        Serial.printf("Status: %d ", WiFi.status()); 
        Serial.println(wl_status_to_string(WiFi.status()));
        WiFiConStatus = 0; // NO AP no STA connected
    }

    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(1000);
    //     Serial.println("Connecting to WiFi...");
    // }
    // Serial.println("Connected to WiFi");
}

void WiFiConfig::handleRoot(AsyncWebServerRequest* request) {
    // debug
    Serial.println("enter handle root");
    String html = "<html><body><h1>Configure WiFi</h1>";
    html += "<b1>Teknosofen</b1>";
    html += "<form method='POST' action='/save'>";
    html += "SSID: <select name='ssid'>";
    html += scanNetworks(); 
    html += "</select><br>";
    html += "Password: <input type='password' name='password'><br>";
    html += "MQTT IP: <input type='MQTTIP' name='MQTTIP'><br>";
    html += "MQTT Port: <input type='MQTTPORT' name='MQTTPORT'><br>";
    html += "MQTT User: <input type='MQTTUser' name='MQTTUser'><br>";
    html += "MQTT PW: <input type='MQTTPW' name='MQTTPW'><br>";

    html += "<input type='submit' value='Save'>";
    html += "</form></body></html>";
    request->send(200, "text/html", html);
}


// void WiFiConfig::handleRoot(AsyncWebServerRequest* request) {
//     String html = "<html><body><h1>Configure WiFi</h1>";
//     html += "<b1>Teknosofen</b1>";
//     html += "<form method='POST' action='/save'>";
//     html += "SSID: <input type='text' name='ssid'><br>";
//     html += "Password: <input type='password' name='password'><br>";
//     html += "<input type='submit' value='Save'>";
//     html += "</form></body></html>";
//     request->send(200, "text/html", html);
// }

String WiFiConfig::scanNetworks(int maxMsPerChan, int channel) { 
    String options = ""; 
    Serial.print("Starting network scan with maxMsPerChan: "); 
    Serial.print(maxMsPerChan); 
    Serial.print(" and channel: "); 
    Serial.println(channel);
    // WiFi.disconnect();
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.mode(WIFI_AP_STA);

    int n = WiFi.scanNetworks(false, false, channel, maxMsPerChan);
    Serial.println("Scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < n; ++i) {
            Serial.printf("%2d", i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4ld", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2ld", WiFi.channel(i));
            Serial.print(" | ");
            String EncryptionTypeStr = "";
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN:            EncryptionTypeStr = "open"; break;
                case WIFI_AUTH_WEP:             EncryptionTypeStr = "WEP"; break;
                case WIFI_AUTH_WPA_PSK:         EncryptionTypeStr ="WPA"; break;
                case WIFI_AUTH_WPA2_PSK:        EncryptionTypeStr ="WPA2"; break;
                case WIFI_AUTH_WPA_WPA2_PSK:    EncryptionTypeStr ="WPA+WPA2"; break;
                case WIFI_AUTH_WPA2_ENTERPRISE: EncryptionTypeStr ="WPA2-EAP"; break;
                case WIFI_AUTH_WPA3_PSK:        EncryptionTypeStr ="WPA3"; break;
                case WIFI_AUTH_WPA2_WPA3_PSK:   EncryptionTypeStr ="WPA2+WPA3"; break;
                case WIFI_AUTH_WAPI_PSK:        EncryptionTypeStr ="WAPI"; break;
                default:                        EncryptionTypeStr ="unknown";
            }
            Serial.println (EncryptionTypeStr);
            options += "<option value='";
            options += WiFi.SSID(i);
            options += "'>";
            options += WiFi.SSID(i);
            options += " (";
            options += WiFi.RSSI(i);
            options += " dBm) ";
            options += EncryptionTypeStr;
            options += "</option>";
        }
    }
    return options;
}

void WiFiConfig::handleSave(AsyncWebServerRequest* request) {
    String ssidStr, passwordStr;
    if (request->hasParam("ssid", true)) {
        ssidStr = request->getParam("ssid", true)->value();
        if (ssidStr.length() > 0) {
            EEPROM.writeString(ssidAddress, ssidStr);
            EEPROM.commit();
            Serial.printf("SSID saved: %s\n", ssidStr.c_str());
        } else {
            Serial.println("SSID not saved");
        }
    }
    if (request->hasParam("password", true)) {
        passwordStr = request->getParam("password", true)->value();
        if (passwordStr.length() > 0) {
            EEPROM.writeString(passwordAddress, passwordStr);
            EEPROM.commit();
            Serial.printf("Password saved: %s\n", passwordStr.c_str());
        } else {
            Serial.println("Password not saved");   
        }
    }
// test stuff
    if (request->hasParam("MQTTIP", true)) {
        String MQTTIPStr = request->getParam("MQTTIP", true)->value();
        if (MQTTIPStr.length() > 0) {
            EEPROM.writeString(mqttIPAddress, MQTTIPStr);
            EEPROM.commit();
            Serial.printf("MQTT IP (almost) saved: %s\n", MQTTIPStr.c_str());
        } else {
            Serial.println("MQTT IP not saved");
        }
    }
    if (request->hasParam("MQTTPORT", true)) {
        String MQTPORTStr = request->getParam("MQTTPORT", true)->value();
        if(MQTPORTStr.length() > 0) {
            EEPROM.writeString(mqttPortAddress, MQTPORTStr);
            EEPROM.commit();
            Serial.printf("MQTT Port: %s saved\n", MQTPORTStr.c_str());
        } else {
            Serial.println("MQTT Port not saved");
        }
        // Serial.printf("MQTT IP: %s\n", MQTPORTStr.c_str());
        // EEPROM.writeString(mqttPortAddress, MQTPORTStr);
    }

// Add selection of MQTT IP-address and portas well
    // EEPROM.writeString(ssidAddress, ssidStr);
    // Serial.printf("SSID saved: %s\n", ssidStr.c_str());
    // EEPROM.writeString(passwordAddress, passwordStr);
    // EEPROM.commit();
    request->send(200, "text/html", "<html><body><h1>Saved! Restart the device.</h1></body></html>");
}

String WiFiConfig::wl_status_to_string(int status) {
  switch (status) {
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN";
  }
}

bool WiFiConfig::getMyWiFiStatus() {
    return bool(WiFi.isConnected());
}
String WiFiConfig::getMySelectedSSID() {
    return WiFi.SSID();
}

String WiFiConfig::getMySelectedIP() {
    return WiFi.localIP().toString();
}

String WiFiConfig::getMyAPIP() {
    return  WiFi.softAPIP().toString();
}

String WiFiConfig::getMyAPSSID() {
    return  apSSID;
}

uint8_t WiFiConfig::getMyWiFiConStatus() {
 return WiFiConStatus;
}