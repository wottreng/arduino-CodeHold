/*
   connections: vcc&ch_pd 3.3v, gpio15 to ground, gnd to ground
   serial rx,tx at 115200 baud
   ESP03:26MHz crystal, 1MB flash
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
#include "DHT.h"

//int conStatus = WL_IDLE_STATUS;
//const char ssid1 = "admin";
//const char pswd1 = "1234567890";
//const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
//DNSServer dnsServer;
ESP8266WebServer server(80);  //listens for incoming tcp traffic
//=============================================================
// Set client WiFi credentials **needs eeprom write/read**
String WIFI_SSID;
String WIFI_PASS;
//===============================================================
//relay pins: use pins 12,13,14,15
//old: pin0=12,pin1=13,pin2=14,pin3=5
//new: pin0=5,pin1=13,pin2=12,pin3=14
//fan -------------
const byte relayPin0 = 5;  //relay pin, D6 | pin status logic, Low=ground=relay on
//heat --------------
const byte relayPin1 = 13;  //relay pin, D7
//ac ---------------
const byte relayPin2 = 12;  //relay pin, D5
//aux - DHT sensor relay ----------------
const byte relayPin3 = 14;  //relay pin, D1 , connect sensor to always on relay position *****
// extra pin-----------------------
//const byte sensorPin = 4;  //mcu pin, D2
//----------------------
//AP config
const String APssid = "HomeHVAC";
const String APpswd = "1234567890";
byte chan = 1;  // wifi channel
int hidden = false;
byte max_con = 6;
const byte maxTry = 50;  //try this many times to connect
//EEPROM setup
byte ssidLength;
byte passLength;
// TEMP SENSOR STUFF ------------------
//relay temp vars
float targetTemp = 67.0;
int ACorHeat = 0;  //0=off, 1=heat, 2=ac
int fanState = 0;  //0=auto, 1=on
float tempBuffer = 0.5;
float upperLimit;
float lowerLimit;
byte pinState;
//DHT sensor -------------
#define DHTPIN 2  // D4 on mcu
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float dhtTemp = 0.0;
float dhtHumid = 0.0;
// 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
//misc
byte x = 0;
byte timer = 0;
const byte LEDpin = 16;  //onboard LED pin
byte LEDstate;           //LED pin status logic, LOW=ON
// timer
unsigned long oldtime = millis();
unsigned long newtime = 0;
unsigned long lastOff = 0;
//=========================================================
void setup() {
    LEDstate = LOW;  //on
    digitalWrite(LEDpin, LEDstate);
    //-----outputs-----------------
    pinMode(relayPin0, OUTPUT);
    digitalWrite(relayPin0, HIGH);  //pin start state
    pinMode(relayPin1, OUTPUT);
    digitalWrite(relayPin1, HIGH);  //pin start state
    pinMode(relayPin2, OUTPUT);
    digitalWrite(relayPin2, HIGH);  //pin start state
    pinMode(relayPin3, OUTPUT); // temp sensor
    digitalWrite(relayPin3, HIGH);  //pin start state
    //-----
    delay(10);
    //-------serial--------------------------------
    //Serial.begin(9600); //***************************
    //delay(100);
    //------sta setup---------------------------------
    WiFi.mode(WIFI_AP_STA);                                      //WIFI_AP_STA dual mode
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
    //read wifi creds from eeprom---------------------
    EEPROM.begin(512);  //eeprom is 512bytes in size
    delay(100);
    ssidLength = EEPROM.read(0x32);  //byte 50
    //WIFI_SSID = "";
    for (int i = 0; i < ssidLength; i++) {
        byte a = EEPROM.read(0x33 + i);
        WIFI_SSID += char(a);
    }
    passLength = EEPROM.read(0x50);  //byte 70
    //WIFI_PASS = "";
    for (int i = 0; i < passLength; i++) {
        byte a = EEPROM.read(0x51 + i);
        WIFI_PASS += char(a);
    }
    //start DHT11 sensor
    delay(10);
    dht.begin();
    //temp range setup
    upperLimit = targetTemp + tempBuffer;
    lowerLimit = targetTemp - tempBuffer;
    //-----connect to WiFi station-------------------
    //try to connect to wifi based on eeprom read cred
    delay(10);
    connectWifi();
    allOff();
    LEDstate = HIGH;  //off
    digitalWrite(LEDpin, LEDstate);
}

//====MAIN LOOP==============================================
void loop() {
  newtime = millis();
  server.handleClient();
  if ((newtime - oldtime) > 30000) {
      checkDHT11();
      manageRelays();
      oldtime = millis();
  }
}
//===================================================
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//==sub functions=================================
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

//control relays
void manageRelays() {
    //relay0=fan,relay1=heat,relay2=ac,relay3=aux
    //onTemp, offTemp, ACorHeat, fanState
    //dhtHumid, dhtTemp
    if (ACorHeat > 0) {   //ac or heat on       
      if (ACorHeat == 1) { //heat selected
        //if a/c is on -> turn off
        if (digitalRead(relayPin2) == LOW) {  
            digitalWrite(relayPin2, HIGH);
            lastOff = millis();
        }
        // manage heat relay ---
        if (dhtTemp < lowerLimit) {
            //turn relay on
            if (digitalRead(relayPin1) == HIGH & millis() - lastOff > 300000) {
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
      } else if (ACorHeat == 2) {  //ac selected           
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
          ACorHeat = 0;
      }
    } else {  //ac and heat are off
        ACorHeat = 0; //error catch in HVAC setup
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
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    delay(300);
    // Read temperature as Celsius (the default)
    //float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float t = dht.readTemperature(true);
    //Serial.println("dht temp ");
    //Serial.println(dhtTemp);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)){
      if (tempReceived == 0){
      resetDHTsensor();
      }
      dhtTemp = 00.00;
      dhtHumid = 00.00;
      tempReceived = tempReceived + 1;
      delay(1000);
    }
    else{
      tempReceived = 10;
      dhtHumid = h;
      dhtTemp = t;
    }
  }
  // error catch if sensor dies
  if(tempReceived != 10){
    allOff();
  }
}
// -- reset DHT sensor by switching relay ---
void resetDHTsensor() {
    digitalWrite(relayPin3, LOW);
    delay(400);
    digitalWrite(relayPin3, HIGH);
}
//===main page builder=========================
void htmlOutput() {
    String htmlRes;
    htmlRes += "======================================</br>";
    htmlRes +=
            " <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"10; url='/'\"/></head>"  //<meta http-equiv=\"refresh\" content=\"5; url='/'\"/>
            "<script type=\"text/JavaScript\"> function timeRefresh(timeoutPeriod) {setTimeout(\"location.reload(true);\", timeoutPeriod);} </script>"
            " <h1>ESP8266 NodeMCU</h1><h3>Home HVAC edition</h3><body><pre style=\"word-wrap: break-word;white-space: pre-wrap;\"></pre></body>"  //onLoad=\"JavaScript:timeRefresh(12000);\"
            "DHT11 output:</br>Humidity: "
            + String(dhtHumid, 2) + "%</br>"
                                    "Temp: "
            + String(dhtTemp, 2) + "F</br>"
                                   "--------------------------</br>"
                                   "Status: ";
    if (ACorHeat == 0) {
        htmlRes += "off</br>";
    } else if (ACorHeat == 1) {
        htmlRes += "Heat</br>";
    } else if (ACorHeat == 2) {
        htmlRes += "A/C</br>";
    } else {
        htmlRes += "Error</br>";
    }

    htmlRes += "--------------------------</br>"
               "target temp: "
               + String(targetTemp, 2) + " F</br>"
                                         "upperLimit: "
               + String(upperLimit, 2) + "F</br>"
                                         "lowerLimit: "
               + String(lowerLimit, 2) + "F</br>";
    byte conStatus = WiFi.status();
    if (conStatus == 0) {
        htmlRes += "Wifi connection is Idle</br>";
    } else if (conStatus == 3) {  //connected
        htmlRes += "Connected to WiFi</br> Connected to AP:&nbsp <b>" + WIFI_SSID + "</b></br>local IP:&nbsp<b>" +
                   WiFi.localIP().toString() + "</b></br>";
    } else if (conStatus == 1) {  //not connected
        htmlRes += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID + "</b></br>";
    } else {
        htmlRes += "<b>Error</b>";
    }
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
               "<br/><p>to change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a>"
               "</br>wifi device can be directly connected to via wifi AP: homeHVAC, password: 1234567890</p><p>Version 1.0</p></html>";
    //send html
    server.send(200, "text/html", htmlRes);  //send string to browser
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
    server.send(200, "text/html", htmlConfig);  //send string to browser
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
    EEPROM.commit();  //flush, write content to eeprom

    String s = "<meta http-equiv=\"refresh\" content=\"30; url='/'\" />"
               "<body> trying to connect to new wifi connection </br> "
               "if connection is successful you will be disconnected from soft AP</br>"
               "please wait a minute then click below to go to main page</body>"
               "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
    server.send(200, "text/html", s);  //Send web page
    delay(200);
    server.stop();
    delay(5);
    connectWifi();
}

//==========================================================
void tempRelaySetup() {
    //ds18Temp, onTemp, offTemp, tempBuffer
    String setuphtml = "<!DOCTYPE HTML><html><head>"
                       "<title>Relay Temp Setup</title>"
                       "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                       "</head><body><p>current relay config: </br>HVAC status: "
                       + String(ACorHeat) + "</br>Target temp: "
                       + String(targetTemp) + "</br></p></b>"
                                              "<form action=\"/relaySetup\" method=\"POST\">"
                                              "<p>HVAC setup :</p>"
                                              "<p><span>[0=off or 1=heat or 2=A/C] </span><input type=\"text\" name=\"status\" value=\""
                       + String(ACorHeat) + "\"</p>"
                                            "<p><span>Target Temp :</span><input type=\"text\" name=\"temp\" value=\""
                       + String(targetTemp) + "\"></p>"
                                              "<p><span>Fan state: </span><select name=\"fanState\"><option value=\"0\">Auto</option><option value=\"1\">On</option></select></p>"
                                              "<p><input type=\"submit\" value=\"Submit\"></p>"
                                              "</form></body></html>";
    String htmlConfig = setuphtml;
    server.send(200, "text/html", htmlConfig);  //send string to browser
}

//=======POST change relay temp values=============================================
void changeConfig() {
    ACorHeat = server.arg("status").toInt();
    targetTemp = server.arg("temp").toFloat();
    if (ACorHeat == 1) {
        //heat
        tempBuffer = 0.5;
    } else if (ACorHeat == 2) {
        //ac
        tempBuffer = 0.3;
    } else {
        tempBuffer = 0.5;
    }
    upperLimit = targetTemp + tempBuffer;
    lowerLimit = targetTemp - tempBuffer;
    fanState = server.arg("fanState").toInt();
    htmlOutput();
}

//-------manual relay control----------------------
void relay0() {
    digitalWrite(relayPin0, !digitalRead(relayPin0));
    server.send(200, "text/html", "relay0: " + String(digitalRead(relayPin0)) + "[0:off,1:on]");
}

void relay1() {
    digitalWrite(relayPin1, !digitalRead(relayPin1));
    server.send(200, "text/html", "relay1: " + String(digitalRead(relayPin1)) + "[0:off,1:on]");
}

void relay2() {
    digitalWrite(relayPin2, !digitalRead(relayPin2));
    server.send(200, "text/html", "relay2 " + String(digitalRead(relayPin2)) + "[0:off,1:on]");
}

void relay3() {
    digitalWrite(relayPin3, !digitalRead(relayPin3));
    server.send(200, "text/html", "relay3 " + String(digitalRead(relayPin3)) + "[0:off,1:on]");
}

//===========================================================================
//===connect to wifi function===================================================
void connectWifi() {
    //Serial.print("Connecting to ");
    //WiFi.setPhyMode(WIFI_PHY_MODE_11B);//11B,11G,11N B=range, N=Mbps
    //Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    x = 0;
    while (x <= maxTry) {
        delay(100);
        Serial.print(".");
        x += 1;
        if (WiFi.status() == WL_CONNECTED) {
            //Serial.println("");
            //Serial.print("Connected to ");
            //Serial.println(WIFI_SSID);
            //Serial.print("IP address: ");
            //Serial.println(WiFi.localIP());
            WiFi.mode(WIFI_STA);  //WIFI_AP_STA dual mode
            x = 100;
        }
        if (WiFi.status() != WL_CONNECTED && x == maxTry) {
            WiFi.mode(WIFI_AP_STA);
            delay(100);
        }
    }
    delay(100);
    //------server--------------------
    server.onNotFound(notFound);  // after ip address /
    server.on("/", HTTP_GET, htmlOutput);
    server.on("/APsetup", HTTP_GET, APsetup);
    server.on("/input", HTTP_POST, handleForm);
    server.on("/tempRelaySetup", HTTP_GET, tempRelaySetup);
    server.on("/relaySetup", HTTP_POST, changeConfig);
    server.on("/relay0", HTTP_GET, relay0);
    server.on("/relay1", HTTP_GET, relay1);
    server.on("/relay2", HTTP_GET, relay2);
    server.on("/relay3", HTTP_GET, relay3);
    server.begin();
    //Serial.println("server turned on");
}

//==========================================
//===no wifi connection===================
/*void wifiNotConnected() {
    x = 0;
    while (x < 10) {
        LEDstate = !LEDstate;
        digitalWrite(LEDpin, LEDstate);
        delay(250);
        x += 1;
    }
}
*/
//==============================================================
void notFound() {  //when stuff after / is incorrect
    String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
               "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
               "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
    server.send(200, "text/html", s); 
}
