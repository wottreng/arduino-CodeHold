/*
   pinMode(, OUTPUT/INPUT);pin# 1,3,15,13,12,14,2,0,4,5,16,9,10
   ADC0: analogRead(A0)
   interupt pins: D0-D8 = pin 16,5,4,
*/
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <HX711.h>

IPAddress apIP(192, 168, 1, 1);
//DNSServer dnsServer;
ESP8266WebServer server(80);  //listens for incoming tcp traffic
//=============================================================
// Set client WiFi credentials **needs eeprom write/read**
String WIFI_SSID;
String WIFI_PASS;
//===============================================================
const byte PIN_TO_SENSOR = 12;   // D6
byte motionDetected = false;
//----------------------
//AP config
const String APssid = "motionSensor";
const String APpswd = "1234567890";
byte chan = 10;  // wifi channel
int hidden = false;
byte max_con = 6;
const byte maxTry = 50;  //try this many times to connect
//EEPROM setup
byte ssidLength;
byte passLength;
const byte LEDpin = 16;  //onboard LED pin, LOW=ON
// timer
unsigned long oldtime = millis();
unsigned long newtime = 0.0;
//unsigned long lastOff = 0.0;
//=========================================================
void setup() {
    pinMode(PIN_TO_SENSOR, INPUT);
    pinMode(LEDpin, OUTPUT);
    delay(100);
    digitalWrite(LEDpin, LOW); //ON
    //-------serial--------------------------------
    //Serial.begin(9600); //***************************
    //delay(100);
    //------sta setup---------------------------------
    WiFi.mode(WIFI_AP_STA); //WIFI_AP_STA dual mode
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
    //-----connect to WiFi station-------------------
    //try to connect to wifi based on eeprom read cred
    delay(10);
    connectWifi();
    digitalWrite(LEDpin, HIGH); //OFF
}

//====MAIN LOOP==============================================
void loop() {
  //newtime = millis();  //oldtime
  server.handleClient();
  
  if (digitalRead(PIN_TO_SENSOR) == HIGH) {   // ON, motion detected
    //Serial.println("Motion detected!");
    motionDetected = true;
    digitalWrite(LEDpin, LOW); //ON
  }
  else{
    if (digitalRead(PIN_TO_SENSOR) == LOW && (millis() - oldtime) > 10000) {   // off, no motion
        // sensor off
        oldtime = millis();
        motionDetected = false;
        digitalWrite(LEDpin, HIGH); //OFF
      }    
  }
}
//===================================================
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++
//==sub functions=================================

//===main page builder=========================
void htmlOutput() {
    String htmlRes;
    htmlRes += "======================================</br>";
    htmlRes +=
            " <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"10; url='/'\"/></head>"  //<meta http-equiv=\"refresh\" content=\"5; url='/'\"/>
            "<script type=\"text/JavaScript\"> function timeRefresh(timeoutPeriod) {setTimeout(\"location.reload(true);\", timeoutPeriod);} </script>"
            " <h1>ESP8266 NodeMCU</h1><h3>Home HVAC edition</h3><body><pre style=\"word-wrap: break-word;white-space: pre-wrap;\"></pre></body>"  //onLoad=\"JavaScript:timeRefresh(12000);\"
            "--------------------------</br>"
            "Status: </br>";
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

    //add buttons and options
    htmlRes += "<br/><p>to change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a></p>"
               "<p>wifi device can be directly connected to via wifi AP: homeHVAC, password: 1234567890</p></html>";
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

void jsonData(){
  String output = "{";
  output += "\"motionDetected\":\"" + String(motionDetected) + "\"";
  output +="}";
  server.send(200, "application/json", output);
}

//===========================================================================
//===connect to wifi function===================================================
void connectWifi() {
    //Serial.print("Connecting to ");
    //WiFi.setPhyMode(WIFI_PHY_MODE_11B);//11B,11G,11N B=range, N=Mbps
    //Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int x = 0;
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
            WiFi.mode(WIFI_AP); //WIFI_AP_STA dual mode
            delay(100);
        }
    }
    delay(100);
    //------server--------------------
    server.onNotFound(notFound);  // after ip address /
    server.on("/", HTTP_GET, htmlOutput);
    server.on("/APsetup", HTTP_GET, APsetup);
    server.on("/input", HTTP_POST, handleForm);
    server.on("/api0", HTTP_GET, jsonData);
    server.begin();
    //Serial.println("server turned on");
}
//==============================================================
void notFound() {  //when stuff after / is incorrect
    String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
               "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
               "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
    server.send(200, "text/html", s); 
}
