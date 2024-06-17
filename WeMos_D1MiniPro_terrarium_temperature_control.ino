#include <ESP8266WiFi.h>  //wifi- library to log the esp8266 into wifi
#include <NTPClient.h>  //NTPClient library is used to get the UTC time from specific time server
#include <WiFiUdp.h>  //User Datagram Protocol. Apparently used for data transfer while grabbing the time.
#include <AutoPID.h>  //AutoPID is a self "learning" PID library
//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>  //not sure, what ESPAsyncTCP and ESPAsyncWebServer do, as the system runs without them too.
#include "TSIC.h"   //library for the TSIC temperature sensor



//interestingly, not every board uses same flash pin -> current boards can be flashed with generic ESP8266 module and flash mode dout

const char* SSID = "FRITZ!Box 7530 JW";
const char* PSK = "73000713234989238567";
//define WiFi login data

const long utcOffsetInSeconds = 7200; //Zeitzonenoffset einstellen    7200 für Sommerzeit, 3600 für Winterzeit

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiServer server(80); //sets up the esp8266 in server configuration, listening on port 80 for upcoming connections
TSIC Sensor1(14); //Input D5, this is the sensorinput for the TSIC temperature sensor, put to port 14, D5 on the D1 Mini Pro. No AnalogIn needed for this

#define OUTPUT_MIN 0
#define OUTPUT_MAX 255 //16bit output
#define KP 70  //startvalue for proportional part of PID- feedback loop
#define KI 0.05  //startvalue for integral part of PID- feedback loop
#define KD 0.0005  //startvalue for differential part of PID- feedback loop

uint16_t temperature = 0;  //initialize a variable for temperature measurements in dataformat of temperature sensor
double input = 0; //input variable will give the temperature in °C read from temperature probe
double temp_setpoint_day = 28; //preset setpoint for temperature during daytime
double temp_setpoint_night = 16; //preset setpoint for temperature during nighttime (this is probably unnecessary, as the temperature is probably not dropping below 16°C anyways
double temp_setpoint_current = temp_setpoint_day;   //initializing the current setpoint as the daytime setpoint
int hour_on = 9;  //set the beginning of daytime
int hour_off = 17; //set the beginning of nighttime
int currenthour;
double output; // variable for the PID output

const int relais = 0;  //pin channel for the relais switch output
bool relaisControl;
String op_mode = "automatic";  //initial value for operating mode: 'automatic', or 'manual'


//sensor model: TSIC 206/ 306  semiconductor sensors T092- housing
//Pin- Layout:
//D5 = Temperature Sensor signal pin (yellow line from sensor cable)
// GND = Temperature Sensor ground pin (white line from sensor cable)
//5V = temperature sensor power supply (brown line from sensor cable)

//Variablen definieren

AutoPIDRelay myPID(&input, &temp_setpoint_current, &relaisControl, 1000 , KP, KI, KD);


void setup() {
  Serial.begin(19200); //open up serial communication via usb with 19200 Bit/s baudrate
  delay(5000); //wait 5s to let everything settle
  pinMode(relais, OUTPUT);  //set pin 0 (D3) to Output to control the relais
  initWiFi();
  myPID.setBangBang(6); //wenn Temp << als setpoint wird OUTPUT_MIN, oder OUTPUT_MAX eingestellt
  myPID.setTimeStep(1000);   //Einstellen, wie oft der PID regeln soll 
  timeClient.begin();
}


void loop() {
  timeClient.update();  //in every loop, update time first
  Serial.println(timeClient.getFormattedTime());  //return the time in serial monitor

  if (timeClient.getHours() >= hour_on && timeClient.getHours() < hour_off) {  //set the current setpoint to the daytime setpoint, when the current hour is matching the daytime value
    temp_setpoint_current = temp_setpoint_day;
    procedure();
  } else {
    temp_setpoint_current = temp_setpoint_night;
    procedure();
  }
}

void updateTemperature(){
  if (Sensor1.getTemperature(&temperature)) { //einfache Temperaturabfrage, also input- Wert
    input = Sensor1.calc_Celsius(&temperature);
  }
}


void procedure() {
  updateTemperature();
  Serial.print(input);
  Serial.println(" °C");
  if (op_mode = "automatic") {
    myPID.run();
    digitalWrite(relais, relaisControl);
  }
  Serial.println(relaisControl);
  
  WiFiClient client = server.available();   //opens up a client that is connected to "server" and has data available for reading
  if (!client) {
    return;   //when no client is detected, the loop gets interrupted here
  }

  if (WiFi.status() != WL_CONNECTED) {
    initWiFi(); //reinitialize wifi, if there is no current connection

  }

  String request = client.readStringUntil('\r'); //request any data, the client has to offer and flush it afterwards
  client.flush();

  if (request == "") {
    client.stop();    //if there is no request, stop the client and return to loop
    return;
  }

  if (request.indexOf("OffAll=1") >= 0) {
    temp_setpoint_day = 0;
    temp_setpoint_night = 0;
    temp_setpoint_current = 0;
    digitalWrite(relais, 0);
    op_mode = "manual";
  }

  client.print(html_gen());

  client.stop();
}



String html_gen() {   //Ausgabe erzeugen
  String output_html;
  output_html += "http/1.1 200 OK\n";
  output_html += "Content-Type: text/html; charset=utf-8\n\n";
  output_html += "<!DOCTYPE html>";
  output_html += "<html>";
  output_html += "<meta http-equiv=\"refresh\" content=\"10;url=http://192.168.178.103\">";
  output_html += String(timeClient.getHours());
  output_html += ":";
  output_html += String(timeClient.getMinutes());
  output_html += " Uhr";
  output_html += "<br>";
  output_html += "operating mode:";
  output_html += op_mode;
  output_html += "<br>";
  output_html += "current Temperature: ";
  output_html += String(input);
  output_html += "&deg C";
  output_html += "<br>";
  output_html += "current Setpoint: ";
  output_html += String(temp_setpoint_current);
  output_html += "&deg C";
  output_html += "<br>";
 
  output_html += "<h1>Weitere Funktionen</h1>";
  output_html += "Heizung ausschalten: ";
  output_html += "<br>";
  output_html += "<form action=\"\" method=\"GET\">";
  output_html += "<button name=\"OffAll\" value=\"1\">OFF</button>";
  output_html += "</form>";
  output_html += "</html>";
  return (output_html);
}
void initWiFi() {
  WiFi.begin(SSID, PSK);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("not connected yet...");
  }

  server.begin();
}
