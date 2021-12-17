#include <Arduino.h>
#include <string.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// custom libs
#include "myWifi.h"
#include <myEEPROM.h>
#include <myF.h>
#include <myWebSocket.h>
#include <myWeather.h>
#include <myTemp.h>

extern bool debug ;

// local variables ---------------

// http server
ESP8266WebServer httpServer(80);

// AP vars
IPAddress apIP(10, 42, 2, 1);
const String APssid = "wifiWeatherStation";
const String APpswd = "";
int chan = 1;
int hidden = false;
int max_con = 4;
// update server
ESP8266HTTPUpdateServer httpUpdater;
const String hostName = "weather";
// wifi
String WIFI_SSID;
String WIFI_PASS;

// eeprom wifi credential location
const byte wifiSSIDloc  = 0;
const byte wifiPSWDloc = 20;

// https client
//const char fingerprint[] PROGMEM =  "B7 CB 1D 1B 02 72 1D 0E 89 A7 94 92 55 38 A7 37 9B 5D CD C4";

// functions ==================================================================

// read wifi creds from eeprom
void myWifiInit(bool debugInput = false){
    if(debugInput)
        debug = true;
    WIFI_SSID = read_eeprom(wifiSSIDloc); // string
    WIFI_PASS = read_eeprom(wifiPSWDloc);
}

// wifi handle client and update MDNS
void myWifiLoop()
{
    httpServer.handleClient();
    MDNS.update();
}

// wifi cred html page
void wifiCredentialUpdatePage() {
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>ESP Input Form</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head><body><p>current wifi AP SSID: &nbsp <b>";
  setuphtml += WIFI_SSID;
  setuphtml += "</b></p>"
               "<form action=\"/input\" method=\"POST\">"
               " Wifi ssid :<br>"
               " <input type=\"text\" name=\"wifi SSID\" value=\"\">"
               " <br>"
               " Wifi password:<br>"
               " <input type=\"password\" name=\"password\" value=\"\">"
               " <br><br>"
               " <input type=\"submit\" value=\"Submit\">"
               "</form> "
               "</body></html>";
  String htmlConfig = setuphtml;
  httpServer.send(200, "text/html", htmlConfig);  //send string to browser
}

// This routine is executed when you change wifi AP
void APinputChange() {
  WIFI_SSID = httpServer.arg("wifi SSID");
  WIFI_PASS = httpServer.arg("password"); //string
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

// check wifi status and connect wifi or else create AP
void connectWifi()
{
    if (WiFi.status() != 3) // if not conneted
    {
        wifi_set_sleep_type(LIGHT_SLEEP_T); // Auto modem sleep is default, light sleep, deep sleep
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.hostname(hostName.c_str());
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        const byte maxTry = 10; // try this many times to connect
        byte x = 0;
        while (x <= maxTry) {
            delay(800);
            if (debug) Serial.print(".");
            byte conStatus = WiFi.status();
            x += 1;
            if (conStatus == 3) {
                WiFi.mode(WIFI_STA); // WIFI_AP_STA dual mode
                WiFi.setOutputPower(20); // 20.5 -> 0 in .25 increments
                x = 100;
                if (debug) {
                    digitalWrite(getLEDpin(), HIGH); // off
                    Serial.println("");
                    Serial.print("Connected to ");
                    Serial.println(WIFI_SSID);
                    Serial.print("IP address: ");
                    Serial.println(WiFi.localIP());
                }
                // enable websocket server
                webSocketServerSwitch(false); // ***********************

            } else if (conStatus != 3 && x == maxTry) {
                // disable socket server
                webSocketServerSwitch(false);
                // access point mode
                WiFi.mode(WIFI_AP);
                WiFi.setOutputPower(16); // 20.5 -> 0 in .25 increments
                WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); //#
                WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
                if (debug) {
                    digitalWrite(getLEDpin(), LOW); // on
                    Serial.println("no AP connection, starting hotspot");
                }
            }
        }
        MDNS.begin(hostName.c_str());

        //------server--------------------
        // server.stop();
        httpServer.onNotFound(notFound); // after ip address /
        httpServer.on("/", HTTP_GET, mainPageHTMLpageBuilder);
        httpServer.on("/APsetup", HTTP_GET, wifiCredentialUpdatePage); // see current AP setup
        httpServer.on("/input", HTTP_POST, APinputChange); // change AP setup
        httpServer.on("/api0", HTTP_GET, jsonData);
        httpServer.on("/reboot", HTTP_GET, reboot);
        httpUpdater.setup(&httpServer);
        httpServer.begin();
        MDNS.addService("http", "tcp", 80);
        if (debug)
            Serial.println("Server started");
    }
}

// url not found
void notFound() {  //when stuff after / is incorrect
  String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
             "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  httpServer.send(200, "text/html", s);  //Send web page
}

void jsonData() {
  String output = "{"
                  "\"LakeTempF\":\"" +
                  String(returnTemp(), 2) + "\"," +
                  // weather data json
                  getWeatherDataJSON();
  if (debug) {
    if (digitalRead(getLEDpin()))
      output += "\"LED\":\"off\"";
    else
      output += "\"LED\":\"on\"";
  }
  output += "\"websocketStatus\":\"" + getSocketStatus() +
            "\","
            "\"Wifi_SSID\":\"" +
            WIFI_SSID + "\",";
  byte mode = WiFi.getMode();
  if (mode == 1)
    output += "\"Wifi_Mode\":\"Station\",";
  else if (mode == 2)
    output += "\"Wifi_Mode\":\"Access Point\",";
  else if (mode == 3)
    output += "\"Wifi_Mode\":\"STA_AP\",";
  else if (mode == 0)
    output += "\"Wifi_Mode\":\"Off\","
              "\"Wifi_Power_dBm\":\"20\","
              "\"Wifi_PhyMode\":\"" +
              String(WiFi.getPhyMode()) +
              "\"," // 3:
              "\"Wifi_Sleep_Mode\":\"" +
              String(WiFi.getSleepMode()) +
              "\"," // 1: auto light_sleep, 2: auto modem sleep
              "\"Wifi_IP_addr\":\"" +
              WiFi.localIP().toString() + "\",";
  byte conStatus = WiFi.status();
  if (conStatus == 0)
    output += "\"Wifi_Status\":\"Idle\",";
  else if (conStatus == 3)
    output += "\"Wifi_Status\":\"Connected\",";
  else if (conStatus == 1)
    output += "\"Wifi_Status\":\"not connected\",";
  else if (conStatus == 7)
    output += "\"Wifi_Status\":\"Disconnected\",";
  else
    output += "\"Wifi_Status\":\"" + String(conStatus) + "\",";
  output += "\"Wifi_Signal_dBm\":\"" + String(WiFi.RSSI()) +
            "\","
            "\"version\":\"" +
            getVersion() +
            "\""
            "}";

  httpServer.send(200, "application/json", output);
}

//===main page builder=========================
void mainPageHTMLpageBuilder()
{
String html = "<!DOCTYPE html>"
"<html lang='en'>"
"<head>"
"<meta charset='UTF-8'>"
"<title>Weather Station</title>"
"<meta name='viewport' content='width=device-width, initial-scale=1'/>"
"<meta http-equiv='refresh' content='20; url='/''  />"
"</head>"
"<style>"
"body {"
"position: relative;"
"top: 0;"
"left: 0;"
"color: white;"
"background-color: transparent;"
"height: calc(100vh - 10px);"
"width: calc(100vw - 10px);"
"border: #ffffff solid 5px;"
"border-radius: 20px;"
"margin: 0;"
"padding: 0;"
"z-index: 0;"
"}"
".flexContainer {"
"background-image: linear-gradient(180deg, #00008B, black);"
"z-index: 1;"
"position: relative;"
"margin: 0;"
"padding: 0;"
"width: 100%;"
"height: 100%;"
"display: flex;"
"text-align: center;"
"align-items: center;"
"align-content: center;"
"flex-wrap: nowrap;"
"flex-direction: column;"
"overflow-y: scroll;"
"font-weight: bold;"
"}"
".title {"
"padding: 10px;"
"font-size: x-large;"
"}"
".subTitle {"
"padding: 5px;"
"font-size: large;"
"}"
".subFlexBlock {"
"z-index: 2;"
"font-size: large;"
"position: relative;"
"left: 0;"
"margin: 0;"
"margin-bottom: 10px;"
"padding: 10px;"
"width: 100%;"
"max-width: 360px;"
"height: auto;"
"display: flex;"
"flex-direction: column;"
"text-align: center;"
"align-items: center;"
"align-content: center;"
"}"
".greenBorder {"
"background-color: black;"
"color: white;"
"width: 90%;"
"border: #FAF0E6 solid 6px;"
"border-radius: 20px;"
"}"
".whiteBorder {"
"background-color: #818181;"
"color: #18453B;"
"width: 80%;"
"border: #ffffff solid 6px;"
"border-radius: 20px;"
"}"
".blackBorder {"
"background-color: #18453B;"
"color: white;"
"width: 85%;"
"border: #000000 solid 6px;"
"border-radius: 20px;"
"}"
".textBlocks {"
"font-size: large;"
"text-align: center;"
"align-content: center;"
"align-items: center;"
"padding: 2px;"
"margin: 0;"
"}"
"button {"
"margin: 0;"
"font-size: large;"
"padding: 5px 10px 5px 10px;"
"color: #b4b4b4;"
"background-color: black;"
"border-radius: 10px;"
"border: #18453B solid 4px;"
"text-decoration: underline #b4b4b4;"
"}"
".greyText{"
"color: #b4b4b4;"
"}"
"</style>"
"<body>"
"<div class='flexContainer'>"
"<div class='title greyText'>LakeHouse Weather Station</div>"
"<div id='date'></div>"
"<script type='text/JavaScript' defer>"
"let a = new Date();"
"let b = String(a).split(' GMT')[0];"
"document.getElementById('date').innerHTML = `${b}`;"
"//console.log(b);"
"</script>"
"<div class='subFlexBlock greenBorder'>"
"<p class='textBlocks greyText'>LAKE TEMP</p>"
"<p class='textBlocks'>"+ String(returnTemp(), 2) +" F</p>"
"</div>"
"<div class='subFlexBlock greenBorder'>"
"<p class='textBlocks greyText'>WIND</p>"
+ getWindDataHTML() +
"</div>"
"<div class='subFlexBlock greenBorder'>"
"<p class='textBlocks greyText'>RAIN</p>"
+ getRainDataHTML() +
"</div>"
"</br></br></br></br></br></br></br></br>"
"<div class='subFlexBlock greenBorder'>";

byte conStatus = WiFi.status();
if (conStatus == 0)  html += "Wifi connection is Idle</br>";
else if (conStatus == 3) html += "Connected to AP:&nbsp <b>"+ WIFI_SSID +"</b></br>"
                                 "local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>"
                                 "Connection Strength: <b>" + String(WiFi.RSSI()) + " dbm</b>";
else if (conStatus == 1) html += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID +  "</b></br>";
else if (conStatus == 7)  html += "Disconnected from access point";
else  html += "<b>Connection Status: "+ String(conStatus) +"</b></br>";

html += "</div>"
/*
"<div class='subFlexBlock whiteBorder'>"
"<span class='textBlocks'>Change Wifi Access Point: </span><a href='/APsetup'><button style='display: block;'>Wifi Setup</button></a>"
"</div>"
*/
"<p>Version " + getVersion() +"</p>"
"</div>"
"</body>"
"</html>";

httpServer.send(200, "text/html", html); // send string to browser
}

void GetRequest() {
  //
  WiFiClient client;
  HTTPClient http;
  const String httpHost = "http://10.42.0.33/";
  //
  if (http.begin(client, httpHost)) {
    int httpCode = http.GET();
    // connected server
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      if (debug) Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        if (debug) {
          String payload = http.getString();
          Serial.println(payload);
        }
      }
    } else {
      if (debug)
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    if (debug)
      Serial.printf("[HTTP} Unable to connect\n");
  }
}
