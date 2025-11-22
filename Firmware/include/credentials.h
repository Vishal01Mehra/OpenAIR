#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <Arduino.h>

// WiFi Credentials Structure
struct WiFiConfig {
    char ssid[32];
    char password[64];
};

// Default credentials (fallback)
const char* DEFAULT_SSID = "OpenFilter-AP";
const char* DEFAULT_PASSWORD = "12345678";

// EEPROM configuration addresses
#define EEPROM_SIZE 512
#define WIFI_CONFIG_ADDR 0
#define RESET_FLAG_ADDR 200

// Sensor configuration
struct SensorConfig {
    bool useRealSensor;
    float calibrationOffset;
    float calibrationMultiplier;
};

#define SENSOR_CONFIG_ADDR 300

#endif