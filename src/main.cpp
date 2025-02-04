#include <Arduino.h>
#include "main.hpp"
#include "constants.hpp"
#include <Wire.h>
#include "WiFiConfig.hpp"
#include "rm67162.h"
#include <TFT_eSPI.h>
#include "AnalogClock.hpp"
#include <DFRobot_BME280.h>
#include "MQTTHandler.hpp"
#include "DryingController.hpp"
#include "HeaterController.hpp"
#include "DisplayManager.hpp"
#include "OTAHandler.hpp"

// #define SDA_PIN 43
// #define SCL_PIN 44

// Graphix CFG
// #define WIDTH  536
// #define HEIGHT 240

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// Analog Clock
AnalogClock myClock(&sprite, CLOCK_XPOS, CLOCK_YPOS, CLOCK_SIZE);

// Display management
DisplayManager displayManager(&tft, &sprite, &myClock);

// Controller setup
// ----------------
DryingController dryingController(INITIAL_SETRH, INITIAL_SETRHHYSTERESIS, DRYER_RELAY);
HeaterController heaterController(INITIAL_SETTEMP, INITIAL_SETTEMPHYSTERESIS, HEATER_RELAY);

// // Create an instance of the MQTTHandler class
// const char* mqttServer = "192.168.1.147"; // Replace with your MQTT broker IP
// const int mqttPort = 1883;                // Default MQTT port
// // String mqttUser = "Teknosofen";
// // String mqttPW = "HassebergsGrund2025";
MQTTHandler mqttHandler(myMqttServer, myMqttPort, &heaterController, &dryingController);

// WIFI CFG
// const int WiFiconfigPin = 15;
WiFiConfig wifiConfig(WiFiconfigPin);

// Temp and humidity setup
typedef DFRobot_BME280_IIC BME;
BME bme(&Wire, 0x77);

// OTA UPDATE
OTAHandler otaHandler;

void printLastOperateStatus(BME::eStatus_t eStatus) {
  switch(eStatus) {
  case BME::eStatusOK: Serial.println("everything ok"); break;
  case BME::eStatusErr: Serial.println("unknown error"); break;
  case BME::eStatusErrDeviceNotDetected: Serial.println("device not detected"); break;
  case BME::eStatusErrParameter: Serial.println("parameter error"); break;
  default: Serial.println("unknown status"); break;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Put this INIT-stuff in a specific function
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);

  pinMode(WiFiconfigPin, INPUT_PULLDOWN);
  pinMode(HEATER_RELAY, OUTPUT);
  digitalWrite(HEATER_RELAY, RELAY_OFF);
  pinMode(DRYER_RELAY, OUTPUT);
  digitalWrite(DRYER_RELAY, RELAY_OFF);
  pinMode(HOTWATER_RELAY, OUTPUT);
  digitalWrite(HOTWATER_RELAY, RELAY_OFF);
  pinMode(SPARE_RELAY, OUTPUT);
  digitalWrite(SPARE_RELAY, RELAY_OFF);
  
  rm67162_init();
  lcd_setRotation(1);
  sprite.setTextColor(TFT_WHITE, TFT_BLACK);
  sprite.createSprite(LCD_WIDTH, LCD_HEIGHT);
  sprite.drawString(VERSION_STRING, 5, 100, 4);
  lcd_PushColors(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t *)sprite.getPointer());
  delay(3000);

  while (bme.begin() != BME::eStatusOK) {
    Serial.println("bme begin failed");
    printLastOperateStatus(bme.lastOperateStatus);
    delay(1000);
  }
  Serial.println("bme begin success");
  delay(200);

  displayManager.begin();
  displayManager.renderHeader();

  int wiFiStatus = wifiConfig.begin(); // Start WiFi configuration
  // make the begin return if it starts as AP or STA

  displayManager.updateWiFiStatus(wiFiStatus, wifiConfig);
  displayManager.renderWiFi();
  if (wiFiStatus == 1) { // If in AP mode
    displayManager.updateActionInfo();
    displayManager.renderActionInfo();
  }
  else if( wiFiStatus != 1 ) { // If not in AP mode     // not in AP mode, Start MQTT
    displayManager.updateWiFiStatus(wiFiStatus, wifiConfig);
    displayManager.renderWiFi();
    mqttHandler.checkAndInitializeEEPROM(); // Check and initialize EEPROM
    mqttHandler.begin();
    displayManager.updateMQTTStatus(mqttHandler);
    displayManager.renderMQTTStatus();

    // Initialize the clock
    myClock.begin();
    // myClock.setRTCTime(); // Set the RTC time with the compile time clock.setRTCTime
    lcd_PushColors(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t *)sprite.getPointer());
  }
  lcd_PushColors(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t *)sprite.getPointer());

  // OTAHandler
  otaHandler.begin();
}

void loop() {
  otaHandler.handle();
 // do a loop that checks if a certain time has eapsed since last time and does not use the %/&%¤%&delay()

static int lastLoopTime = 0;

if (millis() - lastLoopTime > SET_LOOP_TIME) {
  lastLoopTime = millis();

    // Check WiFi status
    // this ought to be a function in the WiFiConfig class...
    int wifiStatus = wifiConfig.getMyWiFiConStatus();
    if (wifiStatus == 0) {
        Serial.println("No Connection");
    } else if (wifiStatus == 1) {
        Serial.printf("AP Mode - SSID: %s, IP: %s\n", wifiConfig.getMyAPSSID(), wifiConfig.getMyAPIP());
    } else if (wifiStatus == 2) {
        Serial.printf("Connected to %s, IP: %s\n", wifiConfig.getMySelectedSSID(), wifiConfig.getMySelectedIP());
    }

    displayManager.updateWiFiStatus(wifiStatus, wifiConfig);
    displayManager.updateMQTTStatus(mqttHandler);

    float temp = bme.getTemperature();
    float press = bme.getPressure() / 100.0;
    float humi = bme.getHumidity();
    displayManager.updateSensorData(temp, press, humi);

    mqttHandler.loop();
    displayManager.updateMQTTStatus(mqttHandler);

    bool heaterOn = heaterController.controlHeater(temp);
    bool dryerOn = dryingController.controlDryer(humi);
    displayManager.updateControllerStatus(heaterOn, dryerOn);

    mqttHandler.publish(mqttHandler.tempTopic, String(temp).c_str());
    mqttHandler.publish(mqttHandler.rhTopic, String(humi).c_str());
    mqttHandler.publish(mqttHandler.pBaroTopic, String(press).c_str());

    Serial.printf("temp: %.1f [C], P: %.1f [kPa], RH: %.1f[%%]\n", temp, press, humi);
    
    heaterController.controlHeater(temp);         // Do the heat and drying controll:
    dryingController.controlDryer(humi);

    mqttHandler.loop();

    char heatStat[5], dryerStat[5];
    sprintf(heatStat, "%d", heaterController.isHeaterOn());
    sprintf(dryerStat, "%d", dryingController.isDryerOn());
    // String heatStat = heaterController.isHeaterOn()? "Heater: ON" : "Heater: OFF";
    // String dryerStat = dryingController.isDryerOn()? "Dryer: ON " : "Dryer: OFF ";
    // mqttHandler.publish("Grunden/Temp", tempStr);              // Publish temperature (Celsius)
    // mqttHandler.publish("Grunden/RH", humiStr);                  // Publish relative humidity (%)
    // mqttHandler.publish("Grunden/Pbaro", pressStr);             // Publish atmospheric pressure (hPa)
    mqttHandler.publish(mqttHandler.heaterStatusTopic, heatStat);        // Publish heater status
    mqttHandler.publish(mqttHandler.dehumidifierStatusTopic, dryerStat); // Publish dehumidifier status

    displayManager.render();

  } 
  // delay(5000);
}
