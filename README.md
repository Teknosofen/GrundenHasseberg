# GrundenHasseberg — Remote Climate Controller

An ESP32-based controller for frost protection and dehumidification at a remote cabin. Monitors temperature, humidity and pressure via a BME280 sensor and controls heater and dehumidifier relays using hysteresis-based logic. Connects to a home MQTT broker for remote monitoring and control.

**Hardware:** LilyGO T-Display-S3 (ESP32-S3) · **Firmware version:** 1.0.6

---

## Features

- Hysteresis-based heater control (default: on below 5 °C, off above 6 °C)
- Hysteresis-based dehumidifier control (default: on above 70 % RH, off below 68 % RH)
- All setpoints adjustable remotely over MQTT and stored in EEPROM
- Live 536×240 AMOLED display with sensor readings, connection status and analog clock
- Automatic WiFi and MQTT reconnection (60-second retry interval)
- OTA firmware updates over WiFi
- Browser-based WiFi/MQTT configuration portal (AP mode)

---

## First-time Setup

The device needs WiFi credentials before it can connect. On first boot (or when GPIO 15 is held HIGH at power-on) it starts in **AP mode**:

1. Connect your phone or laptop to the WiFi network **"Grunden"** (open, no password)
2. Open a browser and navigate to **192.168.4.1**
3. Select your WiFi network from the dropdown, enter the password and (optionally) update the MQTT broker address and port
4. Click **Save**, then power-cycle the device

The device will boot in station mode and connect to your network on all subsequent reboots.

---

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
# Flash via USB (first time / default)
pio run -e usb -t upload

# Flash via OTA (device must be connected to WiFi)
pio run -e ota -t upload
```

In VS Code, switch between `usb` and `ota` by clicking the environment name in the bottom status bar, then click the Upload arrow.

Serial monitor at 115 200 baud:
```bash
pio device monitor
```

---

## MQTT Topics

Broker: `hasseberg.ddns.net:1883`

| Direction | Topic | Content |
|-----------|-------|---------|
| Subscribe | `Grund/status` | `online` / `offline` (LWT, retained) |
| Subscribe | `Grund/uptime` | MQTT uptime in seconds |
| Subscribe | `Grund/temp` | Temperature (°C) |
| Subscribe | `Grund/RH` | Relative humidity (%) |
| Subscribe | `Grund/pbaro` | Pressure (hPa) |
| Subscribe | `Grund/heatStatus` | `1` = heater on, `0` = off |
| Subscribe | `Grund/dehumidStatus` | `1` = dryer on, `0` = off |
| Publish | `Grund/setTemp` | Set target temperature (1–25 °C) |
| Publish | `Grund/setRH` | Set target humidity (0–100 %) |
| Publish | `Grund/setTempHyst` | Set temperature hysteresis (0.1–10 °C) |
| Publish | `Grund/setRHHyst` | Set RH hysteresis (1–20 %) |
| Publish | `Grund/setHeatStatus` | Enable/disable heater (`1`/`0`) |
| Publish | `Grund/setDryStatus` | Enable/disable dryer (`1`/`0`) |
| Publish | `Grund/getStatus` | Request current on/off state (any payload) |
| Publish | `Grund/getSet` | Request all current setpoints (any payload) |

Acknowledgments are published to `Grund/ack` after every received command.

---

## GPIO

| GPIO | Function |
|------|----------|
| 1 | Heater relay (active LOW) |
| 2 | Dehumidifier relay (active LOW) |
| 15 | Pull HIGH at boot → enter AP config mode |
| 43 / 44 | I2C SDA / SCL (BME280) |

---

## Documentation

Full architecture, EEPROM layout, control logic and wiring details: [designDocumentation.md](designDocumentation.md)
