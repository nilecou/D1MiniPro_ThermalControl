#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define DEVICE "ESP8266"
// WiFi Configuration
constexpr const char* SSID = "Tardis_D3";
constexpr const char* PSK = "#20Nls_Jhnns24#";

// Time Configuration
constexpr long UTC_OFFSET_IN_SECONDS = 7200;
constexpr const char* NTP_SERVER = "pool.ntp.org";

// Pins
constexpr int RELAY_PIN = 0;
constexpr int TEMP_SENSOR_PIN = 14;

// Temperature Setpoints
constexpr double DAY_TEMP_SETPOINT = 24.0;
constexpr double NIGHT_TEMP_SETPOINT = 16.0;

// Time Settings
constexpr int HOUR_ON = 6;
constexpr int HOUR_OFF = 15;

// PID Settings
constexpr double KP = 20.0;

// Misc
constexpr int BAUD_RATE = 19200;
constexpr int INIT_DELAY = 5000;
constexpr int HTTP_PORT = 80;

// Modes
constexpr const char* AUTOMATIC_MODE = "automatic";
constexpr const char* MANUAL_MODE = "manual";
constexpr const char* OFF_MODE = "off";

//set up everything needed for data logging
  #define INFLUXDB_URL "https://eu-central-1-1.aws.cloud2.influxdata.com"
  #define INFLUXDB_TOKEN "yTgunM1ZXfetCx6DEgB3tEdYGEFag6Sb_V6EFzgXBwg5T7tBCyTjGNR9ca3yVXAkQgqBf-JvJWuqs5gkuY1YxA=="
  #define INFLUXDB_ORG "b0b04d1df364ff50"
  #define INFLUXDB_BUCKET "Terrarium"
//set timezone info
#define TZ_INFO "GMT-2"


#endif // CONFIG_H
