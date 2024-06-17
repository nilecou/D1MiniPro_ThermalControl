#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
constexpr char* SSID = "FRITZ!Box xxxx";
constexpr char* PSK = "xxxxx";

// Time Configuration
constexpr long UTC_OFFSET_IN_SECONDS = 7200;
constexpr char* NTP_SERVER = "pool.ntp.org";

// Pins
constexpr int RELAY_PIN = 0;
constexpr int TEMP_SENSOR_PIN = 14;

// Temperature Setpoints
constexpr double DAY_TEMP_SETPOINT = 28.0;
constexpr double NIGHT_TEMP_SETPOINT = 16.0;

// Time Settings
constexpr int HOUR_ON = 9;
constexpr int HOUR_OFF = 17;

// PID Settings
constexpr double KP = 70.0;
constexpr double KI = 0.05;
constexpr double KD = 0.0005;
constexpr double BANG_BANG_HYSTERESIS = 6.0;
constexpr unsigned long PID_TIME_STEP = 1000;

// Misc
constexpr int BAUD_RATE = 19200;
constexpr int INIT_DELAY = 5000;
constexpr int HTTP_PORT = 80;

// Modes
constexpr char* AUTOMATIC_MODE = "automatic";
constexpr char* MANUAL_MODE = "manual";

#endif // CONFIG_H
