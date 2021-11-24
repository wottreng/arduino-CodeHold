/*
mcu: esp8266 nodemcu
4 relay breakout board
DHT 22 temp sensor
*/
#include <string.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DHT.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "user_interface.h"
#include <WiFiUdp.h>
#include <NTPClient.h>

bool debug = false;
const String version = "8.1";
/*
version changes: no AP passwd, add NTP, add time dependent changes, api args
*/
//
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", -18000, 60000);
bool timeTempSchedule = false;
String timeChange0[] = {"06:00","68.00"};
String timeChange1[] = {"22:00","66.00"};
//
ESP8266WebServer httpServer(80);  //listens for incoming tcp traffic
ESP8266HTTPUpdateServer httpUpdater;
String hostName = "homeHVAC";
//=============
String WIFI_SSID;
String WIFI_PASS;
//================
//fan -------------
const byte relayPin0 = 5;  //relay pin, D6 | Low=ground=relay on
//heat --------------
const byte relayPin1 = 13;  //relay pin, D7
//ac ---------------
const byte relayPin2 = 12;  //relay pin, D5
//aux - DHT sensor relay ----------------
const byte relayPin3 = 14;  //relay pin, D1 , connect sensor to always on relay position *****
//----------------------
//AP config
IPAddress apIP(192, 168, 1, 1);
const String APssid = hostName;
const String APpswd = "";
byte chan = 1;  // wifi channel
byte hidden = false;
byte max_con = 4;
//EEPROM setup
byte wifiSSIDloc  = 0;
byte wifiPSWDloc = 20;
byte heatOrACLoc = 40;
byte targetTempLoc = 50;
// TEMP SENSOR STUFF ------------------
//relay temp vars
float targetTemp = 67.0; 
byte heatOrAC = 0;  // 0=off, 1=heat, 2=ac
byte fanState = 0;  // 0=auto, 1=on
float tempBuffer = 0.5;
float upperLimit ;
float lowerLimit ;
//DHT sensor -------------
#define DHTPIN 2  // D4 on mcu
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float dhtTemp = 0.0;
float dhtHumid = 0.0;
float hif = 0.0;
// 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
//misc
const byte LEDpin = 16;  //onboard LED pin, LOW=ON
// timer
unsigned long sensorCheckTime = 0;
unsigned long scheduleCheckTime = 0;
unsigned long lastOff = 0;
//=========================================================
void setup() {
    pinMode(LEDpin, OUTPUT);
    delay(100);
    digitalWrite(LEDpin, LOW); // ON
    //-----outputs-----------------
    pinMode(relayPin0, OUTPUT); // fan
    digitalWrite(relayPin0, HIGH);  //pin start state
    pinMode(relayPin1, OUTPUT); // heat
    digitalWrite(relayPin1, HIGH);  //pin start state
    pinMode(relayPin2, OUTPUT); // a/c
    digitalWrite(relayPin2, HIGH);  //pin start state
    pinMode(relayPin3, OUTPUT); // temp sensor
    digitalWrite(relayPin3, HIGH);  //pin start state
    //-------serial--------------------------------
    if(debug) Serial.begin(9600); //***************************
    //read wifi creds from eeprom---------------------
    EEPROM.begin(512);  //eeprom is 512bytes in size
    WIFI_SSID = read_eeprom(wifiSSIDloc); 
    WIFI_PASS = read_eeprom(wifiPSWDloc);
    heatOrAC = read_eeprom(heatOrACLoc).toInt(); 
    targetTemp = read_eeprom(targetTempLoc).toFloat();
    updateTempLimits();
    //start DHT11 sensor
    dht.begin(); // default is 55
    //-----connect to WiFi station-------------------
    connectWifi();
    //
    digitalWrite(LEDpin, HIGH); //OFF
}

//====MAIN LOOP==============================================
void loop() {
  httpServer.handleClient();
  MDNS.update();
  if(WiFi.getMode() == 1) timeClient.update();
  if ((millis() - sensorCheckTime) > 30000) {
    checkDHT11();
    manageRelays();      
    sensorCheckTime = millis();
  }
  if ((millis() - scheduleCheckTime) > 58000 && timeTempSchedule) {
    String time = (timeClient.getFormattedTime()).substring(0,5);
    //String hour = time.substring(0,2);
    //String minute = time.substring(3,2);
    if(time == timeChange0[0]){
        targetTemp = timeChange0[1].toFloat();
        updateTempLimits();
    }
    if(time == timeChange1[0]){
        targetTemp = timeChange1[1].toFloat();
        updateTempLimits();
    }
    scheduleCheckTime = millis();
  }
  delay(200);
}
// ===============================================
//==sub functions=================================

//control relays
void manageRelays() {
    //relay0=fan,relay1=heat,relay2=ac,relay3=aux
    //onTemp, offTemp, heatOrAC, fanState
    //dhtHumid, dhtTemp
    if (heatOrAC > 0) {   //ac or heat on
       //error handling
       if (isnan(dhtHumid) || isnan(dhtTemp)){
            allOff();
            return;
       }
      if (heatOrAC == 1) { //heat selected
        //if a/c is on -> turn off
        if (digitalRead(relayPin2) == LOW) {  
            digitalWrite(relayPin2, HIGH);
            lastOff = millis();
        }
        // manage heat relay ---
        if (dhtTemp < lowerLimit) {
            //turn relay on
            if (digitalRead(relayPin1) == HIGH & millis() - lastOff > 300000) { //5 min
                digitalWrite(relayPin1, LOW);
            }
        } else if (dhtTemp > upperLimit) {
            //turn relay off
            if (digitalRead(relayPin1) == LOW) {
                digitalWrite(relayPin1, HIGH);
                lastOff = millis();
            }
        } else {
            //do nothing
            delay(10);
        }
        // --------------------end of heat
      } else if (heatOrAC == 2) {  //ac selected           
        //if heat is on -> turn off
          if (digitalRead(relayPin1) == LOW) {  
              digitalWrite(relayPin1, HIGH);
              lastOff = millis();
          }
          // manage a/c relay ----------------
          if (dhtTemp > upperLimit) {  // if temp is above limit -> ac on
              //turn relay on
              if (digitalRead(relayPin2) == HIGH & millis() - lastOff > 300000) {
                  digitalWrite(relayPin2, LOW); //on
              }
          } else if (dhtTemp < lowerLimit) {  // if temp is lower than limit -> ac off
              //turn relay off
              if (digitalRead(relayPin2) == LOW) {
                  digitalWrite(relayPin2, HIGH);  //off
                  lastOff = millis();
              }
          } else {
              //do nothing
              delay(100);
          }
          // -------------------------end of a/c
      } else {
          // error catch
          heatOrAC = 0;
      }
    } else {  //ac and heat are off
        heatOrAC = 0; //error catch in HVAC setup
        acAndHeatoff();
        delay(100);
    }
    // fan control -------
    if (fanState == 1) {
        if (digitalRead(relayPin0) == HIGH) {
            digitalWrite(relayPin0, LOW);  //fan on
        }
    } else {
        if (digitalRead(relayPin0) == LOW) {
            digitalWrite(relayPin0, HIGH);  //fan off
        }
    }
}
// DHT11 function
void checkDHT11() {
  int tempReceived = 0;
  while (tempReceived < 4){
    float t = dht.readTemperature(true);
    delay(1000);
    float h = dht.readHumidity();
    hif = dht.computeHeatIndex(dhtTemp, dhtHumid);

    if (isnan(h) || isnan(t)){
      if (tempReceived == 0){
        resetDHTsensor();
      }
      tempReceived = tempReceived + 1;
      delay(500);
    }
    else{
      tempReceived = 10;
      dhtHumid = h;
      dhtTemp = t;
    }
  }
}
// -- reset DHT sensor by switching relay ---
void resetDHTsensor() {
    digitalWrite(relayPin3, LOW);
    delay(800);
    digitalWrite(relayPin3, HIGH);
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
                       "</head><body><p>current wifi AP SSID: &nbsp <b>"
                       + WIFI_SSID + "</b></p>"
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
    httpServer.send(200, "text/html", htmlConfig);  //send string to browser
}
// This routine is executed when you change wifi AP
void changeAP() {
    WIFI_SSID = httpServer.arg("wifi SSID");
    WIFI_PASS = httpServer.arg("password");
    writeToEEPROM(WIFI_SSID,wifiSSIDloc);
    writeToEEPROM(WIFI_PASS,wifiPSWDloc);   

    String s = "<meta http-equiv=\"refresh\" content=\"30; url='/'\" />"
               "<body> trying to connect to new wifi connection </br> "
               "if connection is successful you will be disconnected from soft AP</br>"
               "please wait a minute then click below to go to main page</body>"
               "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
    httpServer.send(200, "text/html", s);  //Send web page
    httpServer.stop();
    connectWifi();
}
//== change on/off temperatures page ==================================
void tempRelaySetup() {
    //ds18Temp, onTemp, offTemp, tempBuffer
    String setuphtml = "<!DOCTYPE HTML><html><head>"
                        "<title>Relay Temp Setup</title>"
                        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                        "</head><body><p>current relay config: </br>HVAC status: " + String(heatOrAC) + "</br>Target temp: " + String(targetTemp) + "</br></p></b>"
                        "<form action=\"/relaySetup\" method=\"POST\">"
                        "<p>HVAC setup :</p>"
                        "<p><span>[0=off or 1=heat or 2=A/C] </span><input type=\"text\" name=\"status\" value=\""
                        + String(heatOrAC) + "\"</p>"
                        "<p><span>Target Temp :</span><input type=\"text\" name=\"temp\" value=\""
                        + String(targetTemp) + "\"></p>"
                        "<p><span>Fan state: </span><select name=\"fanState\"><option value=\"0\">Auto</option><option value=\"1\">On</option></select></p>"
                        "<p><input type=\"submit\" value=\"Submit\"></p>"
                        "</form></body></html>";
    String htmlConfig = setuphtml;
    httpServer.send(200, "text/html", htmlConfig);  //send string to browser
}
//=======POST change relay temp values=============================================
void changeConfig() {
    heatOrAC = httpServer.arg("status").toInt();
    writeToEEPROM(String(heatOrAC), heatOrACLoc);
    targetTemp = httpServer.arg("temp").toFloat();
    writeToEEPROM(String(targetTemp), targetTempLoc);
   
    updateTempLimits();
    fanState = httpServer.arg("fanState").toInt();
    mainPageBuilder();
}
// api variable output
void jsonData(){
    String output = "{"
    "\"heatOrAC\":\"" + String(heatOrAC) +"\","
    "\"fanState\":\"" + String(fanState) +"\","
    "\"targetTemp\":\"" + String(targetTemp, 2) +"\","
    "\"dhtTemp\":\"" + String(dhtTemp, 2) +"\","
    "\"dhtHumid\":\"" + String(dhtHumid, 2) +"\","
    "\"relayPin0\":\"" + String(digitalRead(relayPin0)) +"\","
    "\"relayPin1\":\"" + String(digitalRead(relayPin1)) +"\","
    "\"relayPin2\":\"" + String(digitalRead(relayPin2)) +"\","
    "\"relayPin3\":\"" + String(digitalRead(relayPin3)) +"\","
    "\"lastOff\":\"" + String(lastOff, HEX) +"\","
    "\"time\":\"" + timeClient.getFormattedTime() +"\","
    "\"timeTempSchedule\":\"" + String(timeTempSchedule) +"\","
    "\"timeChange0\":\"" + timeChange0[0] +","+ timeChange0[1] +"\","
    "\"timeChange1\":\"" + timeChange1[0] +","+ timeChange1[1] +"\","
    //output += "\"hour\":\"" + time.substring(0,2) +"\",";
    "\"Wifi_SSID\":\"" + WIFI_SSID +"\",";
    byte mode = WiFi.getMode();
    if (mode = 1) output += "\"Wifi_Mode\":\"Station\",";
    else if (mode = 2) output += "\"Wifi_Mode\":\"Access Point\",";
    else if (mode = 3) output += "\"Wifi_Mode\":\"STA_AP\",";
    else if (mode = 0) output += "\"Wifi_Mode\":\"Off\",";
    output += "\"Wifi_Power_dBm\":\"20\","
    "\"Wifi_PhyMode\":\"" + String(WiFi.getPhyMode()) +"\"," // 3:
    "\"Wifi_Sleep_Mode\":\"" + String(WiFi.getSleepMode()) +"\"," // 1: light_sleep
    "\"Wifi_IP_addr\":\"" + WiFi.localIP().toString() +"\",";
    byte conStatus = WiFi.status();
    if (conStatus == 0) output += "\"Wifi_Status\":\"Idle\",";
    else if (conStatus == 3) output += "\"Wifi_Status\":\"Connected\",";
    else if (conStatus == 1)  output += "\"Wifi_Status\":\"not connected\",";
    else if (conStatus == 7) output += "\"Wifi_Status\":\"Disconnected\",";
    else output += "\"Wifi_Status\":\"" + String(conStatus) +"\",";
    output += "\"Wifi_Signal_dBm\":\"" + String(WiFi.RSSI()) +"\","
    "\"version\":\"" + version + "\""
    "}";

    httpServer.send(200, "application/json", output);
}
//direct api call
void argData(){
    if(httpServer.arg("heatOrAC")!=""){
        byte value = (httpServer.arg("heatOrAC")).toInt();
        if (0 <= value <= 2) heatOrAC = value;
        else heatOrAC = 0;
        writeToEEPROM(String(heatOrAC), heatOrACLoc);
    }
    if(httpServer.arg("targetTemp")!=""){
        float value = (httpServer.arg("targetTemp")).toFloat();
        if (60.0 <= value <= 75.0) targetTemp = value;
        else targetTemp = 69.00;
        writeToEEPROM(String(targetTemp), targetTempLoc);
        updateTempLimits();
    } 
    if(httpServer.arg("fanState")!=""){
        byte value = (httpServer.arg("fanState")).toInt();
        if (0 <= value <= 1) fanState = value;
        else fanState = 0;
    }
    if(httpServer.arg("timeTempSchedule")!=""){
        byte value = (httpServer.arg("timeTempSchedule")).toInt();
        if (value == 1) timeTempSchedule = true;
        else timeTempSchedule = false;
    }
    if(httpServer.arg("timeChange0")!=""){
        String listValue = (httpServer.arg("timeChange0"));
        if (listValue.length() == 11){
            String value0 = listValue.substring(0,5);
            String value1 = listValue.substring(6,11);
            timeChange0[0] = value0;
            timeChange0[1] = value1;
        }
    }
    if(httpServer.arg("timeChange1")!=""){
        String listValue = (httpServer.arg("timeChange1"));
        if (listValue.length() == 11){
            String value0 = listValue.substring(0,5);
            String value1 = listValue.substring(6,11);
            timeChange1[0] = value0;
            timeChange1[1] = value1;
        }
    }

    jsonData();
}
//=== connect to wifi function ===================================================
void connectWifi() {
    WiFi.persistent(false);
    wifi_set_sleep_type(LIGHT_SLEEP_T); // Auto modem sleep is default, light sleep, deep sleep
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostName.c_str());
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    const byte maxTry = 10;//try this many times to connect
    byte x = 0;
    while (x <= maxTry) {
        delay(500);
        if(debug)Serial.print(".");
        byte conStatus = WiFi.status();
        x += 1;
        if (conStatus == 3) {
            WiFi.mode(WIFI_STA);  //WIFI_AP_STA dual mode
            WiFi.setOutputPower(20);
            x = 100;
            digitalWrite(LEDpin, HIGH); //off
        }
       else if(conStatus != 3 && x == maxTry) {
            WiFi.mode(WIFI_AP);
            WiFi.setOutputPower(16);
            WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
            WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
            digitalWrite(LEDpin, LOW); //on
        }
    }
    if(WiFi.getMode() == 1) timeClient.begin();
    MDNS.begin(hostName.c_str());
    //------httpServer--------------------
    httpServer.onNotFound(notFound);  // after ip address /
    httpServer.on("/", HTTP_GET, mainPageBuilder);
    httpServer.on("/APsetup", HTTP_GET, APsetup);
    httpServer.on("/input", HTTP_POST, changeAP);
    httpServer.on("/tempRelaySetup", HTTP_GET, tempRelaySetup);
    httpServer.on("/relaySetup", HTTP_POST, changeConfig);
    httpServer.on("/relay0", HTTP_GET, relay0);
    httpServer.on("/relay1", HTTP_GET, relay1);
    httpServer.on("/relay2", HTTP_GET, relay2);
    httpServer.on("/relay3", HTTP_GET, relay3);
    httpServer.on("/api0", HTTP_GET, argData);
    //httpServer.on("/api1", HTTP_GET, argData);
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    MDNS.addService("http", "tcp", 80);
}
//==============================================================
void notFound() {  //when stuff after / is incorrect
    String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
               "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
               "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
    httpServer.send(200, "text/html", s); 
}
//===main page builder=========================
void mainPageBuilder() {
    String htmlRes;
    htmlRes += "======================================</br>"
            " <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"10; url='/'\"/></head>"  //<meta http-equiv=\"refresh\" content=\"5; url='/'\"/>
            "<script type=\"text/JavaScript\"> function timeRefresh(timeoutPeriod) {setTimeout(\"location.reload(true);\", timeoutPeriod);} </script>"
            " <h1>ESP8266 NodeMCU</h1><h3>Home HVAC edition</h3><body><pre style=\"word-wrap: break-word;white-space: pre-wrap;\"></pre></body>"  //onLoad=\"JavaScript:timeRefresh(12000);\"
            "DHT11 output:</br>Humidity: " + String(dhtHumid, 2) + "%</br>"
            "Temp: " + String(dhtTemp, 2) + "F</br>"
            "--------------------------</br>"
            "Status: ";
    if (heatOrAC == 0) htmlRes += "off</br>";
    else if (heatOrAC == 1) htmlRes += "Heat</br>";
    else if (heatOrAC == 2) htmlRes += "A/C</br>";
    else htmlRes += "Error</br>";

    htmlRes += "--------------------------</br>"
               "target temp: " + String(targetTemp, 2) + " F</br>"
                "upperLimit: " + String(upperLimit, 2) + "F</br>"
                "lowerLimit: " + String(lowerLimit, 2) + "F</br>";
    byte conStatus = WiFi.status();
    if (conStatus == 0) htmlRes += "Wifi connection is Idle</br>";
    else if (conStatus == 3) htmlRes += "Connected to WiFi</br> Connected to AP:&nbsp <b>" + WIFI_SSID + "</b></br>local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>";
    else if (conStatus == 1) htmlRes += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID + "</b></br>";
    else htmlRes += "<b>Error</b>";
    //WL_IDLE_STATUS      = 0,WL_NO_SSID_AVAIL    = 1,WL_SCAN_COMPLETED   = 2,WL_CONNECTED        = 3,
    //WL_CONNECT_FAILED   = 4,WL_CONNECTION_LOST  = 5,WL_DISCONNECTED     = 6

    htmlRes += "</br>======================================</br>";
    if (digitalRead(relayPin0) == LOW) {
        //fan is on
        htmlRes += " <big>fan is on</big><br/>";
    } else {
        //fan is off
        htmlRes += " <big>fan is off</big><br/>";
    }
    if (digitalRead(relayPin1) == LOW) {
        //heat is on
        htmlRes += " <big>heat is on</big><br/>";
    } else {
        //heat is off
        htmlRes += " <big>heat is off</big><br/>";
    }
    if (digitalRead(relayPin2) == LOW) {
        //ac is on
        htmlRes += " <big>A/C is on</big><br/>";
    } else {
        //ac is off
        htmlRes += " <big>A/C is off</big><br/>";
    }
    if (digitalRead(relayPin3) == LOW) {
        //aux is on
        htmlRes += " <big>Aux is on</big><br/>";
    } else {
        //aux is off
        htmlRes += " <big>aux is off</big><br/>";
    }

    //add buttons and options
    htmlRes += "<br/><p>to change HVAC setup: <a href=\"tempRelaySetup\"><button style=\"display: block;\">Temp Setup</button></a>"
               "<br/><p>to change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a></p>"
               "<p>Version "+version+"</p></html>";
    //send html
    httpServer.send(200, "text/html", htmlRes);  //send string to browser
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
//
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
//-------manual relay control----------------------
void relay0() {
    digitalWrite(relayPin0, !digitalRead(relayPin0));
    httpServer.send(200, "text/html", "relay0: " + String(digitalRead(relayPin0)) + "[0:off,1:on]");
}

void relay1() {
    digitalWrite(relayPin1, !digitalRead(relayPin1));
    httpServer.send(200, "text/html", "relay1: " + String(digitalRead(relayPin1)) + "[0:off,1:on]");
}

void relay2() {
    digitalWrite(relayPin2, !digitalRead(relayPin2));
    httpServer.send(200, "text/html", "relay2 " + String(digitalRead(relayPin2)) + "[0:off,1:on]");
}

void relay3() {
    digitalWrite(relayPin3, !digitalRead(relayPin3));
    httpServer.send(200, "text/html", "relay3 " + String(digitalRead(relayPin3)) + "[0:off,1:on]");
}

//turn all relays off
void allOff() {
    //fan off
    if (digitalRead(relayPin0) == LOW) {
        digitalWrite(relayPin0, HIGH);
    }
    //heat off
    if (digitalRead(relayPin1) == LOW) {
        digitalWrite(relayPin1, HIGH);
        lastOff = millis();
    }
    //ac off
    if (digitalRead(relayPin2) == LOW) {
        digitalWrite(relayPin2, HIGH);
        lastOff = millis();
    }
    //aux off
    if (digitalRead(relayPin3) == LOW) {
        digitalWrite(relayPin3, HIGH);
    }
}
//
void acAndHeatoff() {
    if (digitalRead(relayPin1) == LOW) {
        digitalWrite(relayPin1, HIGH);
        lastOff = millis();
    }
    //ac off
    if (digitalRead(relayPin2) == LOW) {
        digitalWrite(relayPin2, HIGH);
        lastOff = millis();
    }
    /* aux off
    if (digitalRead(relayPin3) == LOW) {
        digitalWrite(relayPin3, HIGH);
        relayState3 = HIGH;
    }
    */
}

void updateTempLimits(){
if (heatOrAC == 1) tempBuffer = 0.5; //heat
else if (heatOrAC == 2) tempBuffer = 0.3;  //ac
else tempBuffer = 0.5;

upperLimit = targetTemp + tempBuffer;
lowerLimit = targetTemp - tempBuffer;
}

  /* other helpful commands
  ESP.restart(); //reboot

  */
