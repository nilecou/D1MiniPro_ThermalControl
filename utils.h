#ifndef UTILS_H
#define UTILS_H

#include <ESP8266WiFi.h>
#include <NTPClient.h>

extern double inputTemperature;
extern double currentTempSetpoint;
extern bool relaisControl;
extern String operationMode;

void initializeWiFi() {
  WiFi.begin(SSID, PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Not connected yet...");
  }
  server.begin();
}

bool isDaytime() {
  int currentHour = timeClient.getHours();
  return (currentHour >= HOUR_ON && currentHour < HOUR_OFF);
}

void updateTemperature() {
  uint16_t rawTemperature;
  if (temperatureSensor.getTemperature(&rawTemperature)) {
    inputTemperature = temperatureSensor.calc_Celsius(&rawTemperature);
  }
}

void disableHeating() {
  currentTempSetpoint = 0;
  digitalWrite(RELAY_PIN, LOW);
  operationMode = MANUAL_MODE;
}

String generateHtml() {
  String html = "HTTP/1.1 200 OK\n";
  html += "Content-Type: text/html; charset=utf-8\n\n";
  html += "<!DOCTYPE html><html>";
  html += "<meta http-equiv=\"refresh\" content=\"10;url=http://192.168.178.103\">";
  html += String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + " Uhr<br>";
  html += "Operating mode: " + operationMode + "<br>";
  html += "Current Temperature: " + String(inputTemperature) + "&deg;C<br>";
  html += "Current Setpoint: " + String(currentTempSetpoint) + "&deg;C<br>";
  html += "<h1>Weitere Funktionen</h1>";
  html += "Heizung ausschalten:<br>";
  html += "<form action=\"\" method=\"GET\">";
  html += "<button name=\"OffAll\" value=\"1\">OFF</button>";
  html += "</form></html>";
  return html;
}

#endif // UTILS_H
