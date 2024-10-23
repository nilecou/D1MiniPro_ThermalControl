#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "TSIC.h"
#include "config.h"

double inputTemperature;
double currentTempSetpoint;
String operationMode;

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_IN_SECONDS);

WiFiServer server(HTTP_PORT);
TSIC temperatureSensor(TEMP_SENSOR_PIN);

// Safety parameters
const double MAX_TEMP = 30.0; // Maximum safe temperature
const double MIN_TEMP = 5.0;  // Minimum safe temperature
bool errorState = false;
int dutyCycle = 0;


void setup() {
  Serial.begin(BAUD_RATE);
  delay(INIT_DELAY);
  pinMode(RELAY_PIN, OUTPUT);
  
  // Set the PWM frequency to 100 Hz
  analogWriteFreq(100);
  
  initializeWiFi();
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  timeClient.begin();
  operationMode = AUTOMATIC_MODE;

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
   } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.println(WiFi.localIP());
}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());

  currentTempSetpoint = (isDaytime()) ? DAY_TEMP_SETPOINT : NIGHT_TEMP_SETPOINT;
  controlTemperature();
  writeDB();

  delay(5000);
}

void controlTemperature() {
    updateTemperature();
    Serial.print("Current Temperature: ");
    Serial.println(inputTemperature);
    Serial.print("Setpoint: ");
    Serial.println(currentTempSetpoint);

    // Error Handling: Check for sensor malfunction or overheating
    if (inputTemperature > MAX_TEMP || inputTemperature < MIN_TEMP || isnan(inputTemperature)) {
        disableHeating();
        errorState = true;
        Serial.println("Error: Temperature out of bounds or sensor error");
        return;
    } else {
        errorState = false;
    }

    // Check if it's nighttime and disable heating if true
    if (!isDaytime()) {
        disableHeating();
        errorState = false;
        dutyCycle = 0;
        Serial.println("Nighttime: Heating disabled");
        return;
    }else{
      // Automatically switch back to AUTOMATIC_MODE when daytime starts
      if (isDaytime() && operationMode != AUTOMATIC_MODE) {
        controlON();
        Serial.println("Daytime: Switching back to AUTOMATIC_MODE");
      }
    }

    if (operationMode == AUTOMATIC_MODE && !errorState) {
        double error = currentTempSetpoint - inputTemperature;

        // Proportional control logic: calculate the duty cycle
        dutyCycle = KP * error;

        // Clamp the duty cycle between 0 and 256 (corresponds to 0-100% PWM)
        dutyCycle = constrain(dutyCycle, 0, 256);

        // Apply PWM to the relay pin
        analogWrite(RELAY_PIN, dutyCycle);

        Serial.print("Relay Duty Cycle: ");
        Serial.println(dutyCycle);
    } else {
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Temperature control inactive");
    }
}

void writeDB(){
  Point sensor("temperature");
  sensor.clearFields();
  sensor.addField("temp", inputTemperature);
  sensor.addField("SP", currentTempSetpoint);
  sensor.addField("PWMValue", dutyCycle);
  sensor.addField("ErrorState", int(errorState));
  
  Serial.println(client.pointToLineProtocol(sensor));
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}
//
//void handleClientRequests() {
//  WiFiClient client = server.available();
//  if (!client) return;
//
//  if (WiFi.status() != WL_CONNECTED) initializeWiFi();
//
//  String request = client.readStringUntil('\r');
//  client.flush();
//
//  if (request.isEmpty()) {
//    client.stop();
//    return;
//  }
//
//  if (request.indexOf("OffAll=1") >= 0) {
//    killHeating();
//  }
//
//  if (request.indexOf("mode=1") >= 0) {
//    controlON();
//  }
//
//  client.print(generateHtml());
//  client.stop();
//}

void initializeWiFi() {
  WiFi.begin(SSID, PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Not connected yet...");
  }
  Serial.println("Connected to WiFi");
  server.begin();
}

bool isDaytime() {
  int currentHour = timeClient.getHours();
  return (currentHour >= HOUR_ON && currentHour < HOUR_OFF);
}

void updateTemperature() {
  const int maxRetries = 5;
  double tempSum = 0;
  uint16_t rawTemperature;
  bool success = false;
  for (int attempt = 0; attempt < maxRetries; attempt++) {
      if (temperatureSensor.getTemperature(&rawTemperature)) {
          double temp = temperatureSensor.calc_Celsius(&rawTemperature);
          if (!isnan(temp) && temp < 50 && temp > 10){
              success = true;
              inputTemperature = temp;
              break;
          }
      }
      delay(10); // Short delay before retrying
  }

  if (success == false) {
      inputTemperature = NAN;
      Serial.println("All temperature readings failed.");
  }
}

void disableHeating() {
  currentTempSetpoint = 0;
  analogWrite(RELAY_PIN, 0); // Turn off the relay
  operationMode = MANUAL_MODE;
}

void killHeating() {
  currentTempSetpoint = 0;
  analogWrite(RELAY_PIN, 0); // Turn off the relay
  operationMode = OFF_MODE;
}

void controlON() {
  currentTempSetpoint = (isDaytime()) ? DAY_TEMP_SETPOINT : NIGHT_TEMP_SETPOINT;
  operationMode = AUTOMATIC_MODE;
}
