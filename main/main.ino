#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <AutoPID.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include "TSIC.h"
#include "config.h"

double inputTemperature;
double currentTempSetpoint;
bool relaisControl;
String operationMode;

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, UTC_OFFSET_IN_SECONDS);

WiFiServer server(HTTP_PORT);
TSIC temperatureSensor(TEMP_SENSOR_PIN);

AutoPIDRelay myPID(&inputTemperature, &currentTempSetpoint, &relaisControl, 20000, KP, KI, KD);

Point sensor("temperature");

// Safety parameters
const double MAX_TEMP = 40.0; // Maximum safe temperature
const double MIN_TEMP = 5.0;  // Minimum safe temperature
bool errorState = false;

void setup() {
  Serial.begin(BAUD_RATE);
  delay(INIT_DELAY);
  pinMode(RELAY_PIN, OUTPUT);
  initializeWiFi();
  // myPID.setBangBang(BANG_BANG_on, BANG_BANG_off);   //I'm not sure, if Bang_Bang control works properly (or is set properly) trying with bang bang disabled
  myPID.setTimeStep(PID_TIME_STEP);
  timeClient.begin();
  operationMode = AUTOMATIC_MODE;
  sensor.addTag("device", DEVICE);
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
  handleClientRequests();
}

void controlTemperature() {
    updateTemperature();
    Serial.print("Current Temperature: ");
    Serial.println(inputTemperature);
    Serial.print("Setpoint: ");
    Serial.println(currentTempSetpoint);
    Serial.print("Relay State: ");
    Serial.println(relaisControl);
    
    // Error Handling: Check for sensor malfunction or overheating
    if (inputTemperature > MAX_TEMP || inputTemperature < MIN_TEMP || isnan(inputTemperature)) {
        disableHeating();
        errorState = true;
        Serial.println("Error: Temperature out of bounds or sensor error");
    } else {
        errorState = false;
        if (operationMode != OFF_MODE){
          operationMode = AUTOMATIC_MODE;
        }
    }

    // Check if it's nighttime and disable heating if true
    if (!isDaytime()) {
        disableHeating();
        errorState = false;
        Serial.println("Nighttime: Heating disabled");
        return;
    }

    if (operationMode == AUTOMATIC_MODE && !errorState) {
        myPID.run();
        digitalWrite(RELAY_PIN, relaisControl);
        Serial.println("Temperature control active");
        Serial.println(myPID.getPulseValue());
    } else {
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("Temperature control inactive");
        Serial.println(operationMode);
    }
    Serial.println("At setpoint?");
    Serial.println(myPID.atSetPoint(1));
}


void writeDB(){
  sensor.clearFields();
  sensor.addField("temp", inputTemperature);
  sensor.addField("SP", currentTempSetpoint);
  sensor.addField("RelayState", int(relaisControl));
  sensor.addField("ErrorState", int(errorState));
  Serial.println(client.pointToLineProtocol(sensor));
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
}
  delay(100);  //delay to keep temp-sensor from putting out NaN's
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
    killHeating();
  }

  if (request.indexOf("mode=1") >=0) {
    controlON();
  }
  client.print(generateHtml());
  client.stop();
}

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
  digitalWrite(RELAY_PIN, LOW);
  operationMode = MANUAL_MODE;
  relaisControl = 0;
}

void killHeating(){
  currentTempSetpoint = 0;
  digitalWrite(RELAY_PIN, LOW);
  operationMode = OFF_MODE;
  relaisControl = 0;
}

void controlON() {
  currentTempSetpoint = (isDaytime()) ? DAY_TEMP_SETPOINT : NIGHT_TEMP_SETPOINT;
  operationMode = AUTOMATIC_MODE;
}

String generateHtml() {
  String html = "HTTP/1.1 200 OK\n";
  html += "Content-Type: text/html; charset=utf-8\n\n";
  html += "<!DOCTYPE html><html>";
  html += "<meta http-equiv=\"refresh\" content=\"10;url=http://192.168.178.162\">";
  html += String(timeClient.getHours()) + ":" + String(timeClient.getMinutes()) + " Uhr<br>";
  html += "Operating mode: " + operationMode + "<br>";
  html += "Current Temperature: " + String(inputTemperature) + "&deg;C<br>";
  html += "Current Setpoint: " + String(currentTempSetpoint) + "&deg;C<br>";
  html += "<h1>Weitere Funktionen</h1>";
  html += "Heizung ausschalten:<br>";
  html += "<form action=\"\" method=\"GET\">";
  html += "<button name=\"OffAll\" value=\"1\">OFF</button>";
  html += "<br>";
  html += "Regelung einschalten:<br>";
  html += "<form action=\"\" method=\"GET\">";
  html += "<button name=\"mode\" value=\"1\">ON</button>";
  html += "</form></html>";
  return html;
}
