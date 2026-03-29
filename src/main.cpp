#include <Arduino.h>
#include "main.hpp"
#include "constants.hpp"
#include <Wire.h>
#include <esp_task_wdt.h>
#include <time.h>
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
  // Serial.printf("BME status: %d", bme.begin());
  Serial.println("bme begin success");
  // delay(200);

  displayManager.begin();
  // displayManager.renderHeader();

  int wiFiStatus = wifiConfig.begin(); // Start WiFi configuration, make the begin return if it starts as AP or STA

  setenv("TZ", TZ_INFO, 1); // set timezone once — used by getLocalTime() in AnalogClock
  tzset();

  myClock.begin(); // seeds clock with compile time as fallback (always, incl. AP mode)

  displayManager.updateWiFiStatus(wiFiStatus, wifiConfig);
  if (wiFiStatus == 1) { // AP mode
    displayManager.updateActionInfo();
  }
  else { // STA mode (connected or failed) — start MQTT
    mqttHandler.checkAndInitializeEEPROM();
    mqttHandler.begin();
    displayManager.updateMQTTStatus(mqttHandler);
    displayManager.render();
    Serial.printf("WiFi Status: %d\n", wiFiStatus);

    if (wiFiStatus == 2) {
      configTime(0, 0, NTP_SERVER); // request NTP sync; overrides compile time once resolved
      Serial.println("NTP sync requested");
    }
    lcd_PushColors(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t *)sprite.getPointer());
  }
  lcd_PushColors(0, 0, LCD_WIDTH, LCD_HEIGHT, (uint16_t *)sprite.getPointer());

  // OTAHandler
  otaHandler.begin();

  // Watchdog — reboot if loop stalls for WDT_TIMEOUT_SEC seconds
  esp_task_wdt_init(WDT_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);
}

void loop() {
esp_task_wdt_reset(); // feed watchdog every loop tick
otaHandler.handle();

static unsigned long lastLoopTime      = 0;
static unsigned long lastReconnectAttempt = 0;
static unsigned long wifiConnectedSince   = 0;
static unsigned long wifiLostAt           = 0; // set in 10s block on first drop detection
static bool          reconnecting         = false;
static unsigned long reconnectStarted     = 0;

// Start a non-blocking reconnect every 60 s while WiFi is down (STA mode only)
if (!reconnecting &&
    wifiConfig.getMyWiFiConStatus() != 1 &&
    WiFi.status() != WL_CONNECTED &&
    millis() - lastReconnectAttempt > 60000UL) {
    lastReconnectAttempt = millis();
    reconnectStarted     = millis();
    Serial.println("WiFi: starting reconnect...");
    wifiConfig.beginReconnect(); // returns immediately
    reconnecting = true;
}

// Poll reconnect result — runs every CPU tick, no blocking
if (reconnecting) {
    if (WiFi.status() == WL_CONNECTED) {
        reconnecting = false;
        wifiConfig.onReconnected();
        unsigned long downSecs = (wifiLostAt > 0) ? (millis() - wifiLostAt) / 1000 : 0;
        wifiConnectedSince = millis();
        wifiLostAt = 0;
        Serial.printf("WiFi reconnected! Down for ~%lu s. SSID: %s  IP: %s\n",
            downSecs,
            wifiConfig.getMySelectedSSID().c_str(),
            wifiConfig.getMySelectedIP().c_str());
        configTime(0, 0, NTP_SERVER); // re-sync NTP on reconnect
        otaHandler.restart();
        mqttHandler.loop(); // reconnect MQTT immediately
    } else if (millis() - reconnectStarted > 15000UL) { // 15 s timeout
        reconnecting = false;
        Serial.printf("WiFi reconnect timed out (down ~%lu s) - will retry in 60 s\n",
            (wifiLostAt > 0) ? (millis() - wifiLostAt) / 1000 : 0);
    }
}

if (millis() - lastLoopTime > SET_LOOP_TIME) {
  lastLoopTime = millis();

    // Check WiFi status
    int wifiStatus = wifiConfig.getMyWiFiConStatus();
    if (wifiStatus == 0) {
        if (wifiLostAt == 0 && !reconnecting) wifiLostAt = millis(); // first detection of drop
        Serial.println("WiFi: No Connection");
    } else if (wifiStatus == 1) {
        Serial.printf("WiFi: AP Mode  SSID: %s  IP: %s\n", wifiConfig.getMyAPSSID(), wifiConfig.getMyAPIP());
    } else if (wifiStatus == 2) {
        if (wifiConnectedSince == 0) wifiConnectedSince = millis(); // capture boot connection time
        unsigned long ws = (millis() - wifiConnectedSince) / 1000;
        Serial.printf("WiFi: %s  IP: %s  uptime: %luh%02lum%02lus\n",
            wifiConfig.getMySelectedSSID().c_str(),
            wifiConfig.getMySelectedIP().c_str(),
            ws / 3600, (ws % 3600) / 60, ws % 60);
    }

    displayManager.updateWiFiStatus(wifiStatus, wifiConfig);
    displayManager.updateMQTTStatus(mqttHandler);

    float temp  = bme.getTemperature();
    float press = bme.getPressure() / 100.0;
    float humi  = bme.getHumidity();
    bool sensorOk = (bme.lastOperateStatus == BME::eStatusOK) && !isnan(temp) && !isnan(humi);

    displayManager.updateSensorData(temp, press, humi);
    Serial.printf("temp: %.1f [C], P: %.1f [kPa], RH: %.1f[%%]\n", temp, press, humi);

    mqttHandler.loop(); // check connection, reconnect if needed
    displayManager.updateMQTTStatus(mqttHandler);

    bool heaterOn, dryerOn;
    if (!sensorOk) {
        Serial.printf("BME280 sensor error (%d) - heater and dryer forced OFF\n", bme.lastOperateStatus);
        heaterOn = false;
        dryerOn  = false;
        heaterController.controlHeater(999.0f); // above any threshold → relay OFF
        dryingController.controlDryer(0.0f);    // below any threshold → relay OFF
        if (mqttHandler.isConnected()) mqttHandler.publish("Grund/error", "BME280 sensor failure");
    } else {
        heaterOn = heaterController.controlHeater(temp);
        dryerOn  = dryingController.controlDryer(humi);
    }
    displayManager.updateSensorFault(!sensorOk);
    displayManager.updateControllerStatus(heaterOn, dryerOn);

    if(mqttHandler.isConnected()) {

      unsigned long ms = mqttHandler.getUptimeSeconds();
      Serial.printf("MQTT: %s  uptime: %luh%02lum%02lus\n",
          mqttHandler.getMQTTIP().c_str(),
          ms / 3600, (ms % 3600) / 60, ms % 60);

      mqttHandler.publish(mqttHandler.tempTopic, String(temp).c_str());
      mqttHandler.publish(mqttHandler.rhTopic, String(humi).c_str());
      mqttHandler.publish(mqttHandler.pBaroTopic, String(press).c_str());
      mqttHandler.publish(mqttHandler.uptimeTopic, String(ms).c_str());

      char heatStat[5], dryerStat[5];
      sprintf(heatStat, "%d", heaterController.isHeaterOn());
      sprintf(dryerStat, "%d", dryingController.isDryerOn());
      mqttHandler.publish(mqttHandler.heaterStatusTopic, heatStat);
      mqttHandler.publish(mqttHandler.dehumidifierStatusTopic, dryerStat);

    }
    // mqttHandler.loop();
    displayManager.render();
  } 
}
