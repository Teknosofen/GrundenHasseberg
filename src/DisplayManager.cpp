#include "DisplayManager.hpp"

DisplayManager::DisplayManager(TFT_eSPI* tft, TFT_eSprite* spr, AnalogClock* clock)
    : tft(tft), spr(spr), clock(clock), temperature(0), pressure(0), humidity(0),
      heaterOn(false), dryerOn(false), wifiStatus(0) {}

void DisplayManager::begin() {
    spr->createSprite(WIDTH, HEIGHT);
    clock->begin();
}

void DisplayManager::updateWiFiStatus(int wifiStatus, WiFiConfig& wifiConfig) {
    this->wifiStatus = wifiStatus;
    switch (wifiStatus) {
        case 0:
            wifiDetailsRow1 = "No Connection";
            wifiDetailsRow2 = "";
            break;
        case 1:
            wifiDetailsRow1 = "AP SSID: " + String(wifiConfig.getMyAPSSID());
            wifiDetailsRow2 = "AP IP: " + String(wifiConfig.getMyAPIP());
            break;
        case 2:
            wifiDetailsRow1 = "SSID: " + String(wifiConfig.getMySelectedSSID());
            wifiDetailsRow2 = "IP: " + String(wifiConfig.getMySelectedIP());
            break;
        default:
            wifiDetailsRow1 = "Unknown";
            wifiDetailsRow2 = "";
            break;
    }
}

void DisplayManager::updateMQTTStatus(MQTTHandler& mqttHandler) {
  if (!mqttHandler.isConnected()) {
    mqttDetailsRow1 = "Not Connected ";
    mqttDetailsRow2 = "Yet...";
  } else {
    mqttDetailsRow1 = mqttHandler.getMQTTIP();
    mqttDetailsRow2 = mqttHandler.getMQTTPort();
  }  
}

void DisplayManager::updateActionInfo() {
    actionDetailsRow1 = "select new SSID & PW";
    actionDetailsRow2 = "reboot to connect";
}

void DisplayManager::updateSensorData(float temperature, float pressure, float humidity) {
    this->temperature = temperature;
    this->pressure = pressure;
    this->humidity = humidity;
}

void DisplayManager::updateControllerStatus(bool heaterOn, bool dryerOn) {
    this->heaterOn = heaterOn;
    this->dryerOn = dryerOn;
}

void DisplayManager::render() {
    spr->fillSprite(TFT_BLACK);
    renderHeader();
    renderClock();
    renderWiFi();
    renderSensorData();
    renderControllerStatus();
    renderMQTTStatus();
    lcd_PushColors(0, 0, WIDTH, HEIGHT, (uint16_t*)spr->getPointer());
}

void DisplayManager::renderClock() {
    clock->update();
}

void DisplayManager::renderHeader() {
    spr->setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    spr->drawString("HASSEBERG", firstColStart, firstLine, 4);

    spr->setTextColor(TFT_WHITE, TFT_BLACK);
}

void DisplayManager::renderActionInfo() {
    spr->setTextColor(TFT_RED, TFT_BLACK);
    spr->drawString("Go to SSID & IP above", secondColStart, fifthLine, 4);
    spr->drawString(actionDetailsRow1, secondColStart, sixthLine, 4);
    spr->drawString(actionDetailsRow2, secondColStart, seventhLine, 4);
    spr->setTextColor(TFT_WHITE, TFT_BLACK);
}

void DisplayManager::renderWiFi() {
    spr->drawString("WiFi Status:", secondColStart, firstLine, 4);
    spr->drawString(wifiDetailsRow1, secondColStart, secondLine, 4);
    spr->drawString(wifiDetailsRow2, secondColStart, thirdLine, 4);
}

void DisplayManager::renderMQTTStatus() {
    spr->drawString("MQTT Status:", secondColStart, fifthLine, 4);
    spr->drawString(mqttDetailsRow1, secondColStart, sixthLine, 4);
    spr->drawString(mqttDetailsRow2, secondColStart, seventhLine, 4);
}

void DisplayManager::renderSensorData() {
    char tempStr[10], pressStr[10], humiStr[10];
    sprintf(tempStr, "%.2f", temperature);
    sprintf(pressStr, "%.2f", pressure);
    sprintf(humiStr, "%.2f", humidity);

    spr->drawString("Temp:", firstColStart, secondLine, 4);
    spr->drawString(tempStr, firstColStart + 75, secondLine, 4);
    
    spr->drawString("Pressure:", firstColStart, thirdLine, 4);
    spr->drawString(pressStr, firstColStart + 100, thirdLine, 4);
    
    spr->drawString("Humidity:", firstColStart, fourthLine, 4);
    spr->drawString(humiStr, firstColStart + 110, fourthLine, 4);
}

void DisplayManager::renderControllerStatus() {
    spr->setTextColor(TFT_RED, TFT_BLACK);
    spr->drawString(heaterOn ? "Heater: ON" : "Heater: OFF", firstColStart, sixthLine, 4);
    spr->setTextColor(TFT_SKYBLUE, TFT_BLACK);
    spr->drawString(dryerOn ? "Dryer: ON" : "Dryer: OFF", firstColStart, seventhLine, 4);
    spr->setTextColor(TFT_WHITE, TFT_BLACK);
}
