/*
	temperature wifi switch
	
   pinMode(, OUTPUT/INPUT);pin# 1,3,15,13,12,14,2,0,4,5,16,9,10
   ADC0: analogRead(A0)
   interupt pins: D0-D8 = pin 16,5,4,

*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <DHT.h>

//const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
//DNSServer dnsServer;
ESP8266WebServer server(80);//listens for incoming tcp traffic
String hostName = "bb3000";
//=======
String WIFI_SSID;
String WIFI_PASS;
// == EEPROM ==
byte onTempLoc = 100;
byte offTempLoc = 120;
byte wifiSSIDloc  = 0;
byte wifiPSWDloc = 20;
//======
//relay & LED config
const byte relayPin = 5;//relay pin, D1
const byte LEDpin = 16; //LED pin,  LOW=ON
//AP config
const String APssid = "BB3000";
const String APpswd = "123456789";
byte chan = 10; // wifi channel
const int hidden = false;
byte max_con = 4;
const byte maxTry = 10;//try this many times to connect
// TEMP SENSOR Relay
float onTemp = 0.0;
float offTemp = 1.0;
//DHT sensor
#define DHTPIN 13 // D7
#define DHTTYPE DHT22 // white
DHT dht(DHTPIN, DHTTYPE);
byte tempReadError = false;
float dhtTemp = 0.0;
float dhtHumid = 0.0;
float hif = 0.0;
// 10K resistor from data pin to power pin of the sensor?
// timer
unsigned long oldtime = 0.0;
//=========================================================
void setup() {
  // inputs ----  
  pinMode(13, INPUT_PULLUP);
  //-----outputs-----------------
  pinMode(relayPin, OUTPUT);
  delay(10);
  pinMode(LEDpin, OUTPUT);
  digitalWrite(relayPin, LOW); //pin start state, HIGH = ON
  digitalWrite(LEDpin, LOW); //LED pin start state, LOW = ON
  delay(10);
  //-------serial--------------------------------
  //Serial.begin(9600); //***************************
  //delay(100);
  //------sta setup---------------------------------
  //WiFi.mode(WIFI_AP);//WIFI_AP_STA dual mode
  //WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
  //WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
  //read wifi creds from eeprom---------------------
  EEPROM.begin(512);//eeprom is 512bytes in size
  delay(10);
  WIFI_SSID = read_eeprom(wifiSSIDloc); //string
  delay(10);
  WIFI_PASS = read_eeprom(wifiPSWDloc);
  delay(10);
  String rawOnTemp = read_eeprom(onTempLoc);
  delay(10);
  String rawOffTemp = read_eeprom(offTempLoc);
  delay(10);
  onTemp = rawOnTemp.toFloat();
  offTemp = rawOffTemp.toFloat();
  //start DHT11 sensor
  delay(10);
  dht.begin();
  //-----connect to WiFi station-------------------
  //try to connect to wifi based on eeprom read cred
  delay(10);
  connectWifi();
  MDNS.begin("wifiTempSwitch");
 //digitalWrite(LEDpin, HIGH); //off
}
//==================================================
void loop() {
  server.handleClient();
  if ((millis()-oldtime > 5000)){
      MDNS.update();
  }
  if ((millis()-oldtime)>20000) { //nodemcu is very slow to output page
    checkDHT11();
    manageRelay();
    oldtime = millis();
  }
}
//===================================================
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//==sub functions=================================
//control relay
void manageRelay() {
  if (!tempReadError){
    if(dhtTemp < onTemp) {
    //turn relay on
    digitalWrite(relayPin, HIGH); //on
    digitalWrite(LEDpin, LOW); //ON
    }
    if(dhtTemp > offTemp) {
      //turn relay off
      digitalWrite(relayPin, LOW); //off
      digitalWrite(LEDpin, HIGH); //off
    }
  }else{
    digitalWrite(LEDpin, LOW); //ON
    digitalWrite(relayPin, LOW); // OFF
  }
}
//check temp
void checkDHT11() {
  dhtHumid = dht.readHumidity();
  delay(100);
  dhtTemp = dht.readTemperature(true);
  hif = dht.computeHeatIndex(dhtTemp, dhtHumid);
   if (isnan(dhtHumid) || isnan(dhtTemp) || isnan(hif)) {
    tempReadError = true;
  } else{
    tempReadError = false;
  }
}
//======change pin state for output================================================
void switchprog() {
  if (digitalRead(relayPin) == LOW) {
    digitalWrite(relayPin, HIGH); //ON
    digitalWrite(LEDpin, LOW); //ON
  }
  else {
    digitalWrite(relayPin, LOW); //OFF
    digitalWrite(LEDpin, HIGH); //OFF
  }
  MainPageBuilder();//html output
}
//=====/wifi AP setup page===========================================
void APsetup() {
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>ESP Input Form</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head>"
                     "<body>"
                     "<p>current wifi AP SSID: &nbsp <b>" + WIFI_SSID +  "</b></p>"
                     "<p>current wifi AP pwd: &nbsp <b>" + WIFI_PASS +  "</b></p>"
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
void changeAP() {
  WIFI_SSID = server.arg("wifi SSID");
  WIFI_PASS = server.arg("password"); //string
  writeToEEPROM(WIFI_SSID,wifiSSIDloc);
  writeToEEPROM(WIFI_PASS,wifiPSWDloc);  

  String s = "<meta http-equiv=\"refresh\" content=\"30; url='/'\" />"
             "<body> trying to connect to new wifi connection </br> "
             "if connection is successful you will be disconnected from soft AP</br>"
             "please wait a minute then click below to go to main page</body>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s); //Send web page
  server.stop();
  connectWifi();
}
//==========================================================
void tempRelaySetup() {
  //ds18Temp, onTemp, offTemp, tempBuffer
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>Relay Temp Setup</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head><body><p>current relay config: </br>"
                     "relay on temp: " + String(onTemp) + "</br>"
                     "relay off temp: " + String(offTemp) + "</br>"
                     "</p></b>"
                     "<form action=\"/relaySetup\" method=\"POST\">"
                     " relay sensor trigger : DHT22 Temp Sensor</br>"
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
  String rawOnTemp = server.arg("onTemp"); //String
  String rawOffTemp = server.arg("offTemp");
  onTemp = rawOnTemp.toFloat();
  offTemp = rawOffTemp.toFloat();
  if(onTemp > offTemp){
    onTemp = 69.0;
    offTemp = 70.0;
  } else{
    writeToEEPROM(rawOnTemp, onTempLoc);
    writeToEEPROM(rawOffTemp, offTempLoc);
  }
  MainPageBuilder();
}
//===========================================================================
//===connect to wifi function===================================================
void connectWifi() {
  //server.stop();
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(hostName.c_str());
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int x = 0;
  while (x <= maxTry) {
    delay(500);
    //Serial.print(".");
    x += 1;
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.mode(WIFI_STA);  //WIFI_AP_STA dual mode
        x = 100;
        digitalWrite(LEDpin, HIGH); //off
    }
    if (WiFi.status() != WL_CONNECTED && x == maxTry) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
        WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
        digitalWrite(LEDpin, LOW); //on
    }
  }
  MDNS.begin("bb3000");

  //------server--------------------
  server.onNotFound(notFound);// after ip address /
  server.on("/", HTTP_GET, MainPageBuilder);
  server.on("/relay1", HTTP_GET, switchprog);
  server.on("/APsetup", HTTP_GET, APsetup);
  server.on("/input", HTTP_POST, changeAP); 
  server.on("/tempRelaySetup", HTTP_GET, tempRelaySetup);
  server.on("/relaySetup", HTTP_POST, changeConfig); //secure
  server.on("/api0", HTTP_GET, jsonData);
  server.begin();
  //Serial.println("server turned on");
  MDNS.addService("http", "tcp", 80);
}
//==========================================
void jsonData(){
  String output = "{";
  output += "\"dhtTemp\":\"" + String(dhtTemp, 2) +"\",";
  output += "\"dhtHumid\":\"" + String(dhtHumid, 2) +"\",";
  output += "\"heatIndex\":\"" + String(hif, 2) +"\",";
  output += "\"onTemp\":\"" + String(onTemp, 2) +"\",";
  output += "\"offTemp\":\"" + String(offTemp, 2) +"\",";
   if (digitalRead(LEDpin)){
    // 1 -> off
    output += "\"LED\":\"off\",";
  }else{
    // 0 -> on
    output += "\"LED\":\"on\",";
  }
  if (digitalRead(relayPin)){
    // 1 -> on
    output += "\"relayPinStatus\":\"on\"";
  }else{
    // 0 -> off
    output += "\"relayPinStatus\":\"off\"";
  }
  
  output +="}";
  server.send(200, "application/json", output);
}
//==============================================================
void notFound() { //when stuff after / is incorrect
  String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
             "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s); //Send web page
  //htmlOutput();//redirect back to main page (doesnt work)
}
// -----------------
void MainPageBuilder(){
String html = "";
html += "<html>"
        "<head>";
html += "<meta name='viewport' content='width=device-width, initial-scale=1'/>";
html += "<meta http-equiv='refresh' content='15; url=\"/\"'/>";
html += "</head>";
html += "<style>";
html += "body {";
html += "position: relative;";
html += "top: 0;";
html += "left: 0;";
html += "color: white;";
html += "background-color: transparent;";
html += "height: calc(100vh - 10px);";
html += "width: calc(100vw - 10px);";
html += "border: #ffffff solid 5px;";
html += "border-radius: 20px;";
html += "margin: 0;";
html += "padding: 0;";
html += "z-index: 0;";
html += "}";
html += ".flexContainer {";
html += "background-image: linear-gradient(180deg, #18453B, black);";
html += "z-index: 1;";
html += "position: relative;";
html += "margin: 0;";
html += "padding: 0;";
html += "width: 100%;";
html += "height: 100%;";
html += "display: flex;";
html += "text-align: center;";
html += "align-items: center;";
html += "align-content: center;";
html += "flex-wrap: nowrap;";
html += "flex-direction: column;";
html += "overflow-y: scroll;";
html += "font-weight: bold;";
html += "}";
html += ".title {";
html += "padding: 10px;";
html += "font-size: x-large;";
html += "}";
html += ".subTitle {";
html += "padding: 5px;";
html += "font-size: large;";
html += "}";
html += ".subFlexBlock {";
html += "z-index: 2;";
html += "font-size: large;";
html += "position: relative;";
html += "left: 0;";
html += "margin: 0;";
html += "margin-bottom: 10px;";
html += "padding: 10px;";
html += "width: 100%;";
html += "max-width: 300px;";
html += "height: auto;";
html += "display: flex;";
html += "flex-direction: column;";
html += "text-align: center;";
html += "align-items: center;";
html += "align-content: center;";
html += "}";
html += ".greenBorder {";
html += "background-color: black;";
html += "color: white;";
html += "width: 90%;";
html += "border: #18453B solid 6px;";
html += "border-radius: 20px;";
html += "}";
html += ".whiteBorder {";
html += "background-color: #818181;";
html += "color: #18453B;";
html += "width: 80%;";
html += "border: #ffffff solid 6px;";
html += "border-radius: 20px;";
html += "}";
html += ".blackBorder {";
html += "background-color: #18453B;";
html += "color: white;";
html += "width: 85%;";
html += "border: #000000 solid 6px;";
html += "border-radius: 20px;";
html += "}";
html += ".textBlocks {";
html += "font-size: large;";
html += "text-align: center;";
html += "align-content: center;";
html += "align-items: center;";
html += "padding: 2px;";
html += "margin: 0;";
html += "}";
html += "button {";
html += "margin: 0;";
html += "font-size: large;";
html += "padding: 5px 10px 5px 10px;";
html += "color: #b4b4b4;";
html += "background-color: black;";
html += "border-radius: 10px;";
html += "border: #18453B solid 4px;";
html += "text-decoration: underline #b4b4b4;";
html += "}";
html += ".greyText{";
html += "color: #b4b4b4;";
html += "}";
html += "</style>";
html += "<body>";
html += "<div class='flexContainer'>"
"<div class='title greyText'>Kujawa Baby Baker 3000</div>"
"<div class='subTitle'>Wifi Temperature Switch</div>"
"<div class='subFlexBlock greenBorder'>";
html += "<p class='textBlocks greyText'>ROOM STATUS:</p>";
html += "<p class='textBlocks'>Humidity: " + String(dhtHumid, 2) + "%</p>";
html += "<p class='textBlocks'>Temp: " + String(dhtTemp, 2) + " F</p>";
html += "<p class='textBlocks'>Heat Index: " + String(hif, 2) + " F</p>";
html += "</div>";
html += "<div class='subFlexBlock greenBorder'>";
html += "<p class='textBlocks greyText'>ROOM SETTINGS:</p>";
html += "<p class='textBlocks'>relay on temp: " + String(onTemp, 2) + " F</p>";
html += "<p class='textBlocks'>relay off temp: " + String(offTemp, 2) + " F</p>";
if (digitalRead(relayPin) == HIGH) {
    //relay is ON
    html += "<p class='textBlocks'>relay is <b>ON</b></p>";
  }
  else {
    //relay is OFF
    html += "<p class='textBlocks'>relay is <b>OFF</b></p>";
  }
html += "</div>";
html += "<div class='subFlexBlock whiteBorder'>";
html += "<a href='tempRelaySetup'><button style='display: block;'>Change Room Settings</button></a>";
html += "</div>";
html += "<div class='subFlexBlock blackBorder'>";
html += "<p class='textBlocks'>Manual Switch Control:</p><a href='relay1'><button style='display: block;'><big> ON / OFF </big></button></a>";
html += "</div>";
html += "</br></br></br></br></br></br></br></br>";
html += "<div class='subFlexBlock greenBorder'>";

byte conStatus = WiFi.status();
if (conStatus == 0) {
    html += "Wifi connection is Idle</br>";
  }
  else if (conStatus == 3) {//connected
  html += "Connected to AP:&nbsp <b>"+ WIFI_SSID +"</b></br>"
          "local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>"
          "Connection Strength: <b>"+ WiFi.RSSI() + " dbm</b>";
  }
  else if (conStatus == 1) {//not connected
    html += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID +  "</b></br>";
  }
  else if (conStatus == 7){
    html += "Disconnected from access point";
  }
  else {
    html += "<b>Connection Status: "+ String(conStatus) +"</b></br>";
  }

html += "</div>";
html += "<div class='subFlexBlock whiteBorder'>";
html += "<span class='textBlocks'>Change Wifi Access Point: </span><a href='APsetup'><button style='display: block;'>Wifi Setup</button></a>";
html += "</div>";
html += "<p>Version 1.0</p>";
html += "</div>";
html += "</body>";
html += "</html>";

 server.send(200, "text/html", html);
}

// EEPROM commands =====================================================
void writeToEEPROM(String data,int address){
  byte dataLength = data.length();
  char charInput[dataLength + 1];
  // string to char array
  strcpy(charInput, data.c_str());
  // write to EEPROM
  EEPROM.write(address, dataLength);
  for (int i = 0; i < dataLength; i++) {
    EEPROM.write(address + 1 + i, charInput[i]);
  }
  EEPROM.commit();//flush, write content to eeprom
}

// read eeprom
String read_eeprom(int address) {
  byte dataLength = EEPROM.read(address);
  String data;
  byte a;
    if(dataLength > 20){
      a = 0;
      data += char(a);
  }else{
    for (int i = 0; i < dataLength; i++) {
      a = EEPROM.read(address + 1 + i);
      data += char(a);
    }
  }
  return data;
  }
