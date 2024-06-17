#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AutoPID.h>
#include "TSIC.h"
#include "config.h"
#include "utils.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_IN_SECONDS);

WiFiServer server(HTTP_PORT);
TSIC temperatureSensor(TEMP_SENSOR_PIN);

AutoPIDRelay myPID(&inputTemperature, &currentTempSetpoint, &relaisControl, PID_TIME_STEP, KP, KI, KD);

void setup() {
  Serial.begin(BAUD_RATE);
  delay(INIT_DELAY);
  pinMode(RELAY_PIN, OUTPUT);
  initializeWiFi();
  myPID.setBangBang(BANG_BANG_HYSTERESIS);
  myPID.setTimeStep(PID_TIME_STEP);
  timeClient.begin();
}

void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());

  currentTempSetpoint = (isDaytime()) ? DAY_TEMP_SETPOINT : NIGHT_TEMP_SETPOINT;
  controlTemperature();

  handleClientRequests();
}

void controlTemperature() {
  updateTemperature();
  Serial.print(inputTemperature);
  Serial.println(" °C");

  if (operationMode == AUTOMATIC_MODE) {
    myPID.run();
    digitalWrite(RELAY_PIN, relaisControl);
  }
  Serial.println(relaisControl);
}

void handleClientRequests() {
  WiFiClient client = server.available();
  if (!client) return;

  if (WiFi.status() != WL_CONNECTED) initializeWiFi();

  String request = client.readStringUntil('\r');
  client.flush();

  if (request.isEmpty()) {
    client.stop();
    return;
  }

  if (request.indexOf("OffAll=1") >= 0) {
    disableHeating();
  }

  client.print(generateHtml());
  client.stop();
}
