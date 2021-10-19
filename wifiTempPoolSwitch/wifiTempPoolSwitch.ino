/*
   connections: vcc&ch_pd -3.3v, gpio15 to ground, gnd to ground
   to flash: gpio0 to ground on boot only
   serial rx,tx at 115200 baud
   ESP03:26MHz crystal, 1MB flash

   to flash: click upload and when its trying to connect,
   unplug esp8266, hold flash button, replug in

   pinMode(, OUTPUT/INPUT);pin# 1,3,15,13,12,14,2,0,4,5,16,9,10
   ADC0: analogRead(A0)
   interupt pins: D0-D8 = pin 16,5,4,

*/
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
//#include "DHT.h"

//int conStatus = WL_IDLE_STATUS;
//const char ssid1 = "admin";
//const char pswd1 = "1234567890";
//const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
//DNSServer dnsServer;
ESP8266WebServer server(80);//listens for incoming tcp traffic
//=============================================================
// Set client WiFi credentials
String WIFI_SSID;
String WIFI_PASS;
//===============================================================
//relay & LED config
const byte relayPin = 12;//relay pin, D6 on board
byte relayState = LOW;//pin status logic 
const byte LEDpin = 16; //LED pin
byte LEDstate = HIGH; //LED pin status logic, LOW=ON
//AP config
const String APssid = "PoolTemp";
const String APpswd = "1234567890";
byte chan = 10; // wifi channel
int hidden = false;
byte max_con = 10;
const byte maxTry = 50;//try this many times to connect
//EEPROM setup
byte ssidLength;
byte passLength;
// TEMP SENSOR STUFF
// GPIO where the DS18B20 is connected to
const byte oneWireBus = 4; //D2 on board
float ds18Temp = 0.0;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
//relay temp vars
float onTemp = 69.0;
float offTemp = 70.0;
//float tempBuffer = 1.0;
//DHT sensor
//#define DHTPIN 2
//#define DHTTYPE DHT11
//DHT dht(DHTPIN, DHTTYPE);
//float dhtTemp = 0;
//float dhtHumid = 0;
// 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
//misc
//int whichSensor = 0; //0=dht,1=ds18b20, 2=both
byte poolControl = 0;
byte x = 0;
unsigned long oldtime = millis();
unsigned long newtime;
//=========================================================
void setup() {
  //-----outputs-----------------
  pinMode(relayPin, OUTPUT);
  pinMode(LEDpin, OUTPUT);
  digitalWrite(relayPin, relayState); //pin start state
  digitalWrite(LEDpin, LEDstate); //LED pin start state
  delay(10);
  //-------serial--------------------------------
  //Serial.begin(9600); //***************************
  delay(100);
  //------sta setup---------------------------------
  WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA dual mode
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
  WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
  setupServer();
  //read wifi creds from eeprom---------------------
  EEPROM.begin(512);//eeprom is 512bytes in size
  delay(100);
  ssidLength = EEPROM.read(0x32);//byte 50
  //WIFI_SSID = "";
  for (int i = 0; i < ssidLength; i++) {
    byte a = EEPROM.read(0x33 + i);
    WIFI_SSID += char(a);
  }
  passLength = EEPROM.read(0x50);//byte 70
  //WIFI_PASS = "";
  for (int i = 0; i < passLength; i++) {
    byte a = EEPROM.read(0x51 + i);
    WIFI_PASS += char(a);
  }
  //Serial.println(WIFI_SSID);
  
  delay(100);
  sensors.begin(); // Start the DS18B20 sensor

  //-----connect to WiFi station-------------------
  //try to connect to wifi based on eeprom read cred
  delay(100);
  connectWifi();
}
//==================================================
void loop() {
  newtime = millis();
  server.handleClient();
  if (newtime-oldtime > 1000) {
    checkTemp();
    manageRelay();
  }
  if (WiFi.status() != WL_CONNECTED && (newtime-oldtime)>10000) {
    digitalWrite(LEDpin, LOW); // on
  };
  if (newtime - oldtime > 11000){
    oldtime = newtime;
  }
}
//===================================================
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//==sub functions=================================
//check temp
void checkTemp() {
  //temp sensor stuff
  sensors.requestTemperatures();
  //float temperatureC = sensors.getTempCByIndex(0);
  ds18Temp = sensors.getTempFByIndex(0);
}
//control relay
void manageRelay() {
  //ds18Temp, onTemp, offTemp, tempBuffer
  //dhtHumid, dhtTemp
  if (poolControl == 1) {
    if (ds18Temp < onTemp) {
      //turn relay on
      digitalWrite(relayPin, HIGH);
      digitalWrite(LEDpin, LOW);
      relayState = HIGH;
    }
    else if (ds18Temp > offTemp) {
      //turn relay off
      digitalWrite(relayPin, LOW);
      digitalWrite(LEDpin, HIGH);
      relayState = LOW;
    }
    else {
      //do nothing
      delay(5);
    }
  }
  else {
    delay(100);
  }
}

//===main page builder=========================
void htmlOutput() {
  String htmlRes;
  htmlRes += "======================================</br>";
  htmlRes += " <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"8; url='/'\"/> </head>"
             "<script type=\"text/JavaScript\"> function timeRefresh(timeoutPeriod) {setTimeout(\"location.reload(true);\", timeoutPeriod);} </script>"
             " <h1>Pool Heater Control</h1><body><pre style=\"word-wrap: break-word;white-space: pre-wrap;\"></pre></body>" //onLoad=\"JavaScript:timeRefresh(8000);\"
             "Pool Heater Control: ";
  if (poolControl == 1) {
    htmlRes += "On</br>";
  }
  else {
    htmlRes += "Off</br>";
  }
  htmlRes += "--------------------------</br>"
             "DS18B20 output:</br>"
             "TEMP: " + String(ds18Temp, 2) + " F</br>"
             "--------------------------</br>"
             "relay on temp: " + String(onTemp, 2) + " F</br>"
             "relay off temp: " + String(offTemp, 2) + " F</br>"
             "======================================</br></br>";
  byte conStatus = WiFi.status();
  if (conStatus == 0) {
    htmlRes += "Wifi connection is Idle"
               "======================================</br>";
  }
  else if (conStatus == 3) {//connected
    htmlRes += "Connected to internet</br> Connected to AP:&nbsp <b>" + WIFI_SSID +   "</b></br>local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>"
               "======================================</br>";
  }
  else if (conStatus == 1) {//not connected
    htmlRes += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID +  "</b></br>"
               "======================================</br>";
  }
  else {
    htmlRes += "<b>Error</b>";
  }
  //WL_IDLE_STATUS      = 0,WL_NO_SSID_AVAIL    = 1,WL_SCAN_COMPLETED   = 2,WL_CONNECTED        = 3,
  //WL_CONNECT_FAILED   = 4,WL_CONNECTION_LOST  = 5,WL_DISCONNECTED     = 6

  htmlRes += "</br></br>";

  if (relayState == LOW) {
    //relay is off
    htmlRes += " <big><b>Heater is OFF</b></big><br/><br/>";
  }
  else {
    //relay is on
    htmlRes += " <big><b>Heater is ON</b></big><br/><br/>";
    //"<meta http-equiv=\"refresh\" content=\"5; url='/'\" />";
  }
  //add buttons and options
  htmlRes += "<a href=\"relay1\"><button style=\"display: block;\"><big> ON/OFF </big> </button> </a> "
             "<br/><p>Change switch Configuration: <a href=\"tempRelaySetup\"><button style=\"display: block;\">Temp Relay Setup</button></a>"
             "<br/><p>Change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a>"
             "</br>wifi device can be directly connected to via wifi AP: " + APssid + ", password: " + APpswd + "</p><p>Version 1.2</p></html>";
  //send html
  server.send(200, "text/html", htmlRes);//send string to browser
}
//======change pin state for output================================================
void switchprog() {
  if (relayState == LOW) {
    //Serial.println("relay pin is HIGH");
    digitalWrite(relayPin, HIGH);
    digitalWrite(LEDpin, LOW);
    relayState = HIGH;
  }
  else {
    //Serial.println("relay pin is LOW");
    digitalWrite(relayPin, LOW);
    digitalWrite(LEDpin, HIGH);
    relayState = LOW;
  }
  htmlOutput();//html output
}

//=====/wifi AP setup page===========================================
void APsetup() {
  //String router = WiFi.gatewayIP();
  //int8_t scannedNets = WiFi.scanNetworks();//Number of discovered networks
  //WiFi.SSID(uint8_t scannedNets);//return string of network ssid based on input number
  //WiFi.status();//status
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>ESP Input Form</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head><body><p>current wifi AP SSID: &nbsp <b>" + WIFI_SSID +  "</b></p>"
                     "<form action=\"/input\" method=\"POST\">"
                     " Wifi ssid :</br>"
                     " <input type=\"text\" name=\"wifi SSID\" value=\"enter wifi name\">"
                     " </br>"
                     " Wifi password:</br>"
                     " <input type=\"text\" name=\"password\" value=\"enter wifi password\">"
                     " </br></br>"
                     " <input type=\"submit\" value=\"Submit\">"
                     "</form> "
                     "</body></html>";
  String htmlConfig = setuphtml;
  server.send(200, "text/html", htmlConfig);//send string to browser
}
// This routine is executed when you change wifi AP
void handleForm() {
  WIFI_SSID = server.arg("wifi SSID");
  WIFI_PASS = server.arg("password");

  ssidLength = WIFI_SSID.length();
  char ssidInput[ssidLength + 1];
  passLength = WIFI_PASS.length();
  char passInput[passLength + 1];
  // string to char array
  strcpy(ssidInput, WIFI_SSID.c_str());
  strcpy(passInput, WIFI_PASS.c_str());
  // write to EEPROM
  EEPROM.write(0x32, ssidLength);
  for (int i = 0; i < ssidLength; i++) {
    EEPROM.write(0x33 + i, ssidInput[i]);
  }
  EEPROM.write(0x50, passLength);
  for (int i = 0; i < passLength; i++) {
    EEPROM.write(0x51 + i, passInput[i]);
  }
  EEPROM.commit();//flush, write content to eeprom

  String s = "<meta http-equiv=\"refresh\" content=\"30; url='/'\" />"
             "<body> trying to connect to new wifi connection </br> "
             "if connection is successful you will be disconnected from soft AP</br>"
             "please wait a minute then click below to go to main page</body>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s); //Send web page
  delay(200);
  server.stop();
  delay(20);
  connectWifi();
}
//==========================================================
void tempRelaySetup() {
  //ds18Temp, onTemp, offTemp, tempBuffer, poolControl
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>Relay Temp Setup</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head><body><p>current relay config: </br>"
                     "relay on temp: " + String(onTemp) + "</br>"
                     "relay off temp: " + String(offTemp) + "</br>"
                     "</p></b>"
                     "<form action=\"/relaySetup\" method=\"POST\">"
                     " Pool Heater Control :</br>"
                     " <span>[0=Off or 1=On] </span>"
                     " <input type=\"text\" name=\"poolHeaterControl\" value=\"" + String(poolControl) + "\">"
                     " </br>"
                     " relay on temp :</br>"
                     " <input type=\"text\" name=\"onTemp\" value=\"" + String(onTemp) + "\">"
                     " </br>"
                     " relay off temp:</br>"
                     " <input type=\"text\" name=\"offTemp\" value=\"" + String(offTemp) + "\">"
                     " </br></br>"
                     " <input type=\"submit\" value=\"Submit\">"
                     "</form> "
                     "</body></html>";
  String htmlConfig = setuphtml;
  server.send(200, "text/html", htmlConfig);//send string to browser
}
//POST change relay temp values
void changeConfig() {
  poolControl = server.arg("poolHeaterControl").toInt();
  if (poolControl > 1 & poolControl < 0) {
    poolControl = 0;
  }
  onTemp = server.arg("onTemp").toFloat();
  offTemp = server.arg("offTemp").toFloat();
  if (offTemp > 95) {
    offTemp = 95;
  }
  if (onTemp > offTemp) {
    onTemp = 80;
    offTemp = 81;
    poolControl = 0;
  }
  htmlOutput();
}
//===========================================================================
//===connect to wifi function===================================================
void connectWifi() {
  //Serial.print("Connecting to ");
  //WiFi.setPhyMode(WIFI_PHY_MODE_11B);//11B,11G,11N B=range, N=Mbps
  //Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  x = 0;
  while (x < maxTry) {
    delay(200);
    //Serial.print(".");
    x += 1;
    if (WiFi.status() == WL_CONNECTED) {
      //Serial.println("");
      //Serial.print("Connected to ");
      //Serial.println(WIFI_SSID);
      //Serial.print("IP address: ");
      //Serial.println(WiFi.localIP());
      WiFi.mode(WIFI_STA);//WIFI_AP_STA dual mode
      digitalWrite(LEDpin, HIGH); // off
      x = 100;
      delay(100);
      setupServer();
    }
  }
  if (x == (maxTry)) {
    // did not connect ----
    //Serial.print("wifi not connected");
    //wifiNotConnected();
    digitalWrite(LEDpin, LOW); // on
  }
}
//==========================================
/*
//===no wifi connection===================
void wifiNotConnectedDONOTUSE() {
  x = 0;
  while (x < 10) {
    LEDstate = ! LEDstate;
    digitalWrite(LEDpin, LEDstate);
    delay(250);
    x += 1;
  }
}
*/
void setupServer(){
  server.onNotFound(notFound);
  server.on("/", HTTP_GET, htmlOutput);
  server.on("/relay1", HTTP_GET, switchprog);
  server.on("/APsetup", HTTP_GET, APsetup);
  server.on("/input", HTTP_POST, handleForm); 
  server.on("/tempRelaySetup", HTTP_GET, tempRelaySetup);
  server.on("/relaySetup", HTTP_POST, changeConfig); 
  server.on("/data", HTTP_GET, dataOutput);
  server.begin();
}
void dataOutput(){
  checkTemp();
  //poolControl onTemp offTemp
  String output = "{";
  output += "\"tempSensor\":\"" + String(ds18Temp, 2) +"\"";
  output += ",";
  output += "\"ControlStatus\":\"" + String(poolControl) +"\"";
  output += ",";
  output += "\"onTemp\":\"" + String(onTemp) +"\"";
  output += ",";
  output += "\"offTemp\":\"" + String(offTemp) +"\"";

  output += "}";
  server.send(200, "application/json", output); 
}
//==============================================================
void notFound() { //when stuff after / is incorrect
  String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
             "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s); //Send web page
  //delay(2000);
  //htmlOutput();//redirect back to main page (doesnt work)
}

