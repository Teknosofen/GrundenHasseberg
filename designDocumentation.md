# Hasseberg Climate Controller — Design Documentation

**Project:** GrundenHasseberg
**Hardware:** LilyGO T-Display-S3 (ESP32-S3)
**Framework:** Arduino / PlatformIO
**Current Version:** 1.0.6

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Hardware Platform](#2-hardware-platform)
3. [Software Architecture](#3-software-architecture)
4. [Initial WiFi Setup and Configuration](#4-initial-wifi-setup-and-configuration)
5. [WiFi Reconnection Logic](#5-wifi-reconnection-logic)
6. [MQTT Communication](#6-mqtt-communication)
7. [Climate Control Logic](#7-climate-control-logic)
8. [Display System](#8-display-system)
9. [EEPROM Layout](#9-eeprom-layout)
10. [Main Loop and Task Scheduling](#10-main-loop-and-task-scheduling)
11. [Over-the-Air (OTA) Updates](#11-over-the-air-ota-updates)
12. [GPIO Pin Assignment](#12-gpio-pin-assignment)
13. [Library Dependencies](#13-library-dependencies)
14. [Version History](#14-version-history)

---

## 1. Project Overview

The Hasseberg Climate Controller is an ESP32-based embedded system installed at a remote cabin (Hasseberg/Grunden). Its purpose is to maintain a habitable climate in the building by:

- **Controlling a heater** to maintain a minimum temperature (frost protection)
- **Controlling a dehumidifier** to keep relative humidity below a set level

The device connects to a home WiFi network and communicates over MQTT, allowing remote monitoring and control via any MQTT client (e.g., Home Assistant, MQTT Explorer, or a custom app). A local display shows live sensor readings, connection status, and controller state at all times.

---

## 2. Hardware Platform

| Component | Details |
|-----------|---------|
| **Microcontroller** | LilyGO T-Display-S3 (ESP32-S3) |
| **Display** | Built-in 536×240 AMOLED (rm67162 driver) |
| **Sensor** | DFRobot BME280 — temperature, humidity, barometric pressure |
| **Sensor interface** | I2C (SDA = GPIO 43, SCL = GPIO 44, address 0x77) |
| **Relay outputs** | 4 × active-low relay channels (GPIO 1, 2, 3, 10) |
| **WiFi config pin** | GPIO 15 (pulled low; pull HIGH to force AP mode) |
| **Serial monitor** | USB CDC, 115 200 baud |

### Relay Wiring Convention

Relays are **active-low**: `LOW` = relay ON, `HIGH` = relay OFF.
This is reflected in `constants.hpp`:
```cpp
#define RELAY_ON  LOW
#define RELAY_OFF HIGH
```

---

## 3. Software Architecture

The firmware is structured as a set of independent, single-responsibility classes instantiated in `main.cpp`.

```
main.cpp
├── WiFiConfig          — WiFi provisioning, STA connection, AP portal, reconnection
├── MQTTHandler         — MQTT broker connection, pub/sub, message routing
├── HeaterController    — Hysteresis-based heater on/off control
├── DryingController    — Hysteresis-based dehumidifier on/off control
├── DisplayManager      — Sprite-based display rendering (double-buffered)
│   └── AnalogClock     — RTC-backed analog clock drawn into the sprite
└── OTAHandler          — Arduino OTA update listener
```

### Class Dependency Diagram

```
main.cpp
  │
  ├── WiFiConfig (WiFiconfigPin)
  │
  ├── MQTTHandler (broker, port, &HeaterController, &DryingController)
  │     └── PubSubClient (knolleary)
  │
  ├── HeaterController (Tset, hysteresis, HEATER_RELAY)
  ├── DryingController (RHset, hysteresis, DRYER_RELAY)
  │
  ├── DisplayManager (&tft, &sprite, &AnalogClock)
  │     └── AnalogClock (&sprite, x, y, size)
  │
  └── OTAHandler
```

All settings that need to survive a reboot (setpoints, hysteresis, enable/disable flags, WiFi credentials, MQTT address) are persisted in **EEPROM**.

---

## 4. Initial WiFi Setup and Configuration

The WiFi configuration uses a dual-mode design: the device either connects as a **Station (STA)** to an existing network, or starts as an **Access Point (AP)** to allow the user to configure credentials via a browser.

### 4.1 Boot Decision Flow

```
setup()
  │
  └── wifiConfig.begin()
        │
        ├── Read SSID + password from EEPROM
        │
        ├── Is GPIO 15 HIGH  OR  EEPROM empty?
        │       │                       │
        │      YES                      YES
        │       └───────────────────────┘
        │               │
        │           setupAP()  →  WiFiConStatus = 1
        │               │
        │           Start AsyncWebServer on port 80
        │           Serve config portal at "/"
        │           Return status = 1 (AP mode)
        │
        └── NO: connectToWiFi()
                  │
                  ├── WiFi.disconnect()
                  ├── WiFi.setTxPower(8.5 dBm)
                  ├── WiFi.mode(WIFI_STA)
                  ├── WiFi.setHostname("HassebergsGrund")
                  ├── WiFi.begin(ssid, password)
                  │
                  ├── Retry loop: up to 20 × 500 ms = 10 s max
                  │
                  ├── Connected?  → WiFiConStatus = 2, return 2
                  └── Failed?     → WiFiConStatus = 0, return 0
```

### 4.2 AP Configuration Portal

When in AP mode the device broadcasts a WiFi network named **"Grunden"** (open, no password) and hosts a web page at its soft-AP IP address.

The configuration page (`/`) contains:
- A dropdown of scanned nearby WiFi networks (SSID, RSSI, encryption type)
- Password input field
- MQTT broker IP input
- MQTT port input
- MQTT username input
- MQTT password input

On form submit (`POST /save`) all values are written to EEPROM. The page instructs the user to **restart the device** to apply the new settings. There is no automatic reboot — the user must power-cycle or press the reset button.

### 4.3 Display Feedback During Boot

| WiFi result | Display shows |
|-------------|--------------|
| AP mode (status = 1) | AP SSID + AP IP address + action instructions |
| STA connected (status = 2) | Network SSID + assigned IP address |
| No connection (status = 0) | "No Connection" |

---

## 5. WiFi Reconnection Logic

After a successful boot connection, WiFi may drop due to router reboots, range issues, or interference. The firmware handles this with two mechanisms.

### 5.1 Live Status Detection

`getMyWiFiConStatus()` cross-checks its cached `WiFiConStatus` against the live `WiFi.status()` on every call:

```cpp
uint8_t WiFiConfig::getMyWiFiConStatus() {
    if (WiFiConStatus == 2 && WiFi.status() != WL_CONNECTED) {
        WiFiConStatus = 0;   // mark as lost immediately
    }
    return WiFiConStatus;
}
```

This ensures the display correctly shows "No Connection" as soon as the link drops, rather than showing stale boot-time status.

### 5.2 Automatic Reconnection (60-second interval)

An independent timer in `loop()` attempts reconnection every 60 seconds whenever the device is in STA mode and WiFi is not connected:

```
loop()  (runs on every CPU tick)
  │
  ├── Not AP mode  AND  WiFi.status() != WL_CONNECTED  AND  60 s elapsed?
  │       │
  │      YES → wifiConfig.tryReconnect()
  │               │
  │               ├── AP mode? → skip
  │               ├── Already connected? → update status, return true
  │               └── Call connectToWiFi() → full reconnect sequence
  │
  └── (MQTT reconnect handled separately — see §6.2)
```

`tryReconnect()` delegates to the same `connectToWiFi()` used at boot, which runs up to 20 retry attempts (10 seconds max). On success, MQTT will automatically reconnect on the next 10-second loop cycle via `mqttHandler.loop()`.

### 5.3 AP Mode Exception

`tryReconnect()` immediately returns `false` if `WiFiConStatus == 1`. The device never attempts STA reconnection while in AP configuration mode.

---

## 6. MQTT Communication

### 6.1 Broker Configuration

| Parameter | Value |
|-----------|-------|
| Broker hostname | `hasseberg.ddns.net` |
| Port | 1883 |
| Client ID | `Hasseberg` |
| Username | `Teknosofen` |
| Password | stored in `MQTTHandler` |

The broker address and port are defined in `constants.hpp` and can be overridden via the AP configuration portal (stored in EEPROM at addresses 64 and 96).

### 6.2 MQTT Reconnection

`mqttHandler.loop()` is called every 10 seconds. It checks `client.connected()` and calls `reconnect()` if needed:

```cpp
void MQTTHandler::loop() {
    if (!client.connected()) {
        reconnect();          // single attempt, no blocking delay
    }
    client.loop();            // process incoming messages
}
```

On successful reconnection all 11 subscriptions are re-registered automatically.

### 6.3 Subscribed Topics (incoming commands)

| Topic | Action |
|-------|--------|
| `Grund/setTemp` | Set target temperature (1–25 °C) |
| `Grund/setRH` | Set target relative humidity (0–100 %) |
| `Grund/setTempHyst` | Set temperature hysteresis (0.1–10 °C) |
| `Grund/setRHHyst` | Set RH hysteresis (1–20 %) |
| `Grund/getStatus` | Reply with current heater/dryer on-off state |
| `Grund/help` | Reply with full topic list |
| `Grund/getSet` | Reply with all current setpoints |
| `Grund/getHeatStatus` | Reply with heater on/off |
| `Grund/getDryStatus` | Reply with dryer on/off |
| `Grund/setHeatStatus` | Enable (1) / disable (0) heater function |
| `Grund/setDryStatus` | Enable (1) / disable (0) dryer function |

### 6.4 Published Topics (outgoing data)

| Topic | Content | Retained | Published |
|-------|---------|----------|-----------|
| `Grund/status` | `"online"` / `"offline"` | Yes | On connect / on unexpected disconnect (LWT) |
| `Grund/uptime` | MQTT connection uptime in seconds | No | Every 10 s |
| `Grund/temp` | Temperature in °C | No | Every 10 s |
| `Grund/RH` | Relative humidity in % | No | Every 10 s |
| `Grund/pbaro` | Barometric pressure in hPa | No | Every 10 s |
| `Grund/heatStatus` | `1` = heater on, `0` = off | No | Every 10 s |
| `Grund/dehumidStatus` | `1` = dryer on, `0` = off | No | Every 10 s |
| `Grund/ack` | Acknowledgment text for received commands | No | On command receipt |

### 6.5 Device Availability — Last Will and Testament (LWT)

A naive approach to availability monitoring — publishing `"connected"` over MQTT — only works when the connection is already up, so it can never report a lost connection. The solution is **Last Will and Testament (LWT)**: when the device registers with the broker it includes a pre-written "offline" message. If the TCP connection ever drops unexpectedly (power loss, WiFi failure, router reboot), the **broker** publishes that message on the device's behalf, without any involvement from the device.

```
Device boots / reconnects
  └── client.connect(..., willTopic="Grund/status", willMsg="offline", retain=true)
        └── on success: client.publish("Grund/status", "online", retain=true)

Device loses power / WiFi drops
  └── broker detects missing keepalive → publishes "offline" to Grund/status (LWT)

Device reconnects (60 s WiFi timer fires, WiFi restored, MQTT reconnects)
  └── client.publish("Grund/status", "online", retain=true)
```

The retained flag ensures that any client subscribing to `Grund/status` at any time — even long after the device connected — immediately receives the current state without waiting for the next publish cycle.

### 6.6 Connection Uptime Tracking

Both WiFi and MQTT connection uptimes are tracked in the firmware and reported every 10 seconds.

**Serial output (every 10 s):**
```
WiFi: MyNetwork  IP: 192.168.1.42  uptime: 0h03m20s
MQTT: hasseberg.ddns.net  uptime: 0h03m18s
```

**On WiFi reconnect (event-driven):**
```
WiFi disconnected - attempting reconnect...
WiFi reconnected! Down for 127 s. SSID: MyNetwork  IP: 192.168.1.42
MQTT connected
```

**On failed attempt:**
```
WiFi reconnect failed (down 187 s) - will retry in 60 s
```

**MQTT topic:** `Grund/uptime` publishes the MQTT connection uptime in seconds every 10 s. A sudden drop to a low value (e.g. from 3600 back to 5) indicates a reconnection event happened — the device was offline and has just come back.

### 6.7 Message Validation

All incoming setpoint values are range-validated before being applied. Out-of-range values are rejected and an error message is published to `Grund/ack`. Valid values are applied immediately and written to EEPROM so they survive a reboot.

---

## 7. Climate Control Logic

Both controllers implement a simple **hysteresis (dead-band) control** loop, evaluated every 10 seconds.

### 7.1 HeaterController

Controls the heater relay on GPIO 1.

**State machine:**

```
T < Tset  AND  heaterActive == 1  →  heaterOn = true   (turn ON)
T > Tset + hysteresis             →  heaterOn = false  (turn OFF)
heaterActive == 0                 →  heaterOn = false  (force OFF)
```

**Default setpoints (EEPROM-backed):**

| Parameter | Default |
|-----------|---------|
| Target temperature | 5.0 °C |
| Hysteresis | 1.0 °C |
| Active | enabled (1) |

The heater turns on when temperature falls below `Tset` and turns off when temperature rises above `Tset + hysteresis`. This prevents rapid on/off cycling near the setpoint.

### 7.2 DryingController

Controls the dehumidifier relay on GPIO 2.

**State machine:**

```
RH > RHset  AND  dryerActive == 1  →  dryerOn = true   (turn ON)
RH < RHset - hysteresis            →  dryerOn = false  (turn OFF)
dryerActive == 0                   →  dryerOn = false  (force OFF)
```

**Default setpoints (EEPROM-backed):**

| Parameter | Default |
|-----------|---------|
| Target RH | 70 % |
| Hysteresis | 2 % |
| Active | enabled (1) |

### 7.3 Enable/Disable via MQTT

Both controllers have an `active` flag (1 = enabled, 0 = disabled). When disabled, the relay is forced off regardless of sensor readings. This allows remote disabling of either function without changing setpoints, and the state persists across reboots via EEPROM.

---

## 8. Display System

### 8.1 Rendering Architecture

The display uses a **double-buffered sprite** approach to eliminate flicker:
1. All drawing operations target a `TFT_eSprite` (RAM buffer, 536×240 pixels)
2. The completed frame is pushed to the display in one DMA transfer via `lcd_PushColors()`

`DisplayManager::render()` is called once per 10-second loop cycle.

### 8.2 Screen Layout

```
┌─────────────────────────────────────────────────────┐  y=5
│ HASSEBERG          │  WiFi Status:          │[Clock]│
│ Temp:   xx.xx      │  SSID: xxxx            │       │  y=35
│ Pressure: xxx.xx   │  IP: xxx.xxx.xxx.xxx   │       │  y=60
│ Humidity: xx.xx    │                        │       │  y=90
│                    │  MQTT Status:          │       │  y=120
│ Heater: ON/OFF     │  hasseberg.ddns.net    │       │  y=150
│ Dryer:  ON/OFF     │  1883                  │       │  y=180
└─────────────────────────────────────────────────────┘
  col 1: x=5          col 2: x=250            clock: x=474, y=178
```

### 8.3 Color Coding

| Element | Color |
|---------|-------|
| Header "HASSEBERG" | Green-yellow |
| Sensor data | White |
| Heater status | Red |
| Dryer status | Sky blue |
| WiFi / MQTT text | White |

### 8.4 Analog Clock

A small analog clock is rendered in the bottom-right corner (center x=474, y=178, radius=60). The clock is initialized to the **firmware compile time** at boot (using `__DATE__` and `__TIME__` macros via `setRTCTime()`). There is no NTP synchronization — time will drift and reset to compile time on each reboot.

---

## 9. EEPROM Layout

Total size: 512 bytes. Validated by a 4-byte signature (`0xDEADBEEF`) at address 128.

| Address | Size | Content |
|---------|------|---------|
| 0 | 32 bytes | WiFi SSID (string) |
| 32 | 32 bytes | WiFi password (string) |
| 64 | 32 bytes | MQTT broker IP/hostname (string) |
| 96 | 32 bytes | MQTT port (string) |
| 128 | 4 bytes | Signature `0xDEADBEEF` |
| 132 | 4 bytes | Target temperature (float, °C) |
| 136 | 4 bytes | Target RH (float, %) |
| 140 | 4 bytes | Temperature hysteresis (float, °C) |
| 144 | 4 bytes | RH hysteresis (float, %) |
| 148 | 4 bytes | Heater active status (int, 0 or 1) |
| 152 | 4 bytes | Dryer active status (int, 0 or 1) |

If the signature is absent (first boot or EEPROM corruption), default values are written automatically by `checkAndInitializeEEPROM()`.

---

## 10. Main Loop and Task Scheduling

```
loop()  [runs at MCU speed, no blocking]
  │
  ├── otaHandler.handle()              every CPU tick — OTA listener
  │
  ├── WiFi reconnect check             every CPU tick, gated by 60 s timer
  │     └── if WiFi lost → tryReconnect()
  │               └── on success → otaHandler.restart()
  │
  └── if millis() - lastLoopTime > 10 000 ms:   [10-second cycle]
        │
        ├── wifiConfig.getMyWiFiConStatus()      read live WiFi state
        ├── displayManager.updateWiFiStatus()    update display buffer
        ├── displayManager.updateMQTTStatus()    update display buffer
        │
        ├── bme.getTemperature()                 read sensor
        ├── bme.getPressure()                    read sensor
        ├── bme.getHumidity()                    read sensor
        ├── displayManager.updateSensorData()    update display buffer
        │
        ├── heaterController.controlHeater(T)    evaluate & drive relay
        ├── dryingController.controlDryer(RH)    evaluate & drive relay
        ├── displayManager.updateControllerStatus()
        │
        ├── mqttHandler.loop()                   reconnect if needed + client.loop()
        ├── displayManager.updateMQTTStatus()    refresh after reconnect attempt
        │
        ├── if mqttHandler.isConnected():
        │     ├── publish Grund/temp
        │     ├── publish Grund/RH
        │     ├── publish Grund/pbaro
        │     ├── publish Grund/uptime
        │     ├── publish Grund/heatStatus
        │     └── publish Grund/dehumidStatus
        │
        └── displayManager.render()              push sprite to display
```

**Note:** `heaterController.controlHeater()` is called twice per cycle (lines 161 and 167 in `main.cpp`). The second call is used to capture the return value for the display update. Both calls drive the relay to the same state, so there is no functional impact.

---

## 11. Over-the-Air (OTA) Updates

The firmware supports wireless updates via the **Arduino OTA** mechanism. OTA is initialized in `setup()` and serviced on every `loop()` iteration via `otaHandler.handle()` — this is the highest-priority call in `loop()` and runs outside the 10-second gate.

### Initialization

`otaHandler.begin()` registers four callbacks (start, end, progress, error) and calls `ArduinoOTA.begin()`, which opens the UDP listener on port 3232 and registers the mDNS service entry that PlatformIO uses to discover the device.

### Deploying a firmware update

PlatformIO does not auto-discover OTA targets through a GUI port picker. The upload port must be configured explicitly, either permanently in `platformio.ini` or as a one-off on the command line.

**`platformio.ini` is pre-configured with two environments:**

| Environment | Method | Command |
|-------------|--------|---------|
| `usb` (default) | USB cable via esptool | `pio run -e usb -t upload` |
| `ota` | WiFi via espota | `pio run -e ota -t upload` |

In VS Code, click the environment name in the **bottom status bar** to switch between `usb` and `ota`, then click the Upload (right-arrow) button.

The OTA environment uses a static IP address as `upload_port`. The device's IP is shown on the display (WiFi status line) and in the Serial monitor output. Update `platformio.ini` whenever the IP changes (e.g. after a router reboot assigns a new address):

```ini
[env:ota]
upload_protocol = espota
upload_port     = 192.168.x.x
```

> **Note on mDNS (`.local` hostnames):** Although the device advertises itself as `HassebergsGrund.local`, this name resolution is unreliable on Windows for Python-based tools like espota. Use the IP address directly.

**What to expect during upload**

1. PlatformIO connects to the device over UDP port 3232
2. The Serial monitor prints `Start updating sketch` then `Progress: X%`
3. The ESP32 reboots automatically into the new firmware when complete
4. Serial prints `End` just before the reboot

> **Note:** The device must be in STA mode (WiFi connected, `Grund/status = online`) for OTA to work. OTA is not available in AP configuration mode.

### OTA after WiFi reconnection

`ArduinoOTA.begin()` binds the UDP listener and mDNS record to the active network connection at the time it is called. If WiFi drops and reconnects, that binding is no longer valid and OTA uploads would fail.

To handle this, `OTAHandler` exposes a `restart()` method that calls `ArduinoOTA.begin()` again. The reconnection block in `loop()` calls it automatically whenever `tryReconnect()` succeeds:

```cpp
bool reconnected = wifiConfig.tryReconnect();
if (reconnected) {
    otaHandler.restart();   // re-register mDNS + UDP listener on new link
}
```

OTA is therefore available after both the initial boot connection and any subsequent automatic WiFi reconnection, without requiring a manual reboot.

---

## 12. GPIO Pin Assignment

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 1 | Heater relay | Output | Active LOW |
| 2 | Dehumidifier relay | Output | Active LOW |
| 3 | Hot water relay | Output | Active LOW, not currently used |
| 10 | Spare relay | Output | Active LOW, not currently used |
| 15 | WiFi config mode | Input (pull-down) | Pull HIGH at boot → enter AP mode |
| 43 | I2C SDA | Bidirectional | BME280 sensor |
| 44 | I2C SCL | Output | BME280 sensor |
| PIN_LED | Status LED | Output | Set HIGH at boot |

---

## 13. Library Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| `bodmer/TFT_eSPI` | ^2.5.43 | Display driver + sprite rendering |
| `dfrobot/DFRobot_BME280` | ^1.0.2 | Temperature / humidity / pressure sensor |
| `mathieucarbou/ESPAsyncWebServer` | ^3.4.5 | Non-blocking web server for AP config portal |
| `knolleary/PubSubClient` | ^2.8 | MQTT client |
| `espressif32` (platform) | — | ESP32-S3 Arduino core and WiFi/OTA stack |

---

## 14. Version History

| Version | Date | Commit | Description |
|---------|------|--------|-------------|
| 1.0.6 | 2025-06-02 | `ba0cdf7` | Bug fix: improved management of lost MQTT connection; heater and humidity control no longer stops when MQTT connection is lost. |
| 1.0.5 | 2025-06-01 | `42c5d01` | Improved display presentation for WiFi status when connected (shows SSID and IP). |
| 1.0.0 | 2025-02-04 | `7003ec2` | Initial commit — first working build with WiFi, MQTT, climate control, and display. |
| —     | 2025-02-04 | `ab23f85` | Repository initialized. |

### Post-repository changes (applied directly, not yet tagged)

| Change | Description |
|--------|-------------|
| WiFi reconnection | Added `tryReconnect()` method and 60-second automatic reconnection timer. `getMyWiFiConStatus()` now reflects live WiFi state rather than cached boot-time status. Ensures the device recovers from WiFi drops without requiring a reboot. |
