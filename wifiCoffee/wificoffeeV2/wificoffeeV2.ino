/*
   ESP8266 act as AP (Access Point) and simplest Web Server
   to control GPIO 2
   Connect to AP "arduino-er", password = "password"
   Open browser, visit 192.168.1.2 

   connections: vcc&ch_pd -3.3v, gpio15 to ground, gnd to ground
   to flash: gpio0 to ground on boot only
   serial rx,tx at 115200 baud
   ESP03:26MHz crystal, 1MB flash

   to flash: click upload and when its trying to connect,
   unplug esp8266, hold flash button, replug in
*/

#include <ESP8266WiFi.h>
//#include <DNSServer.h>
#include <ESP8266WebServer.h>

const byte relayPin = 12;//relay pin
//const byte Pin2 = 2; //extra pin
byte relayState = LOW;//pin status logic

//const byte DNS_PORT = 53;
//IPAddress apIP(192, 168, 100, 100);
//DNSServer dnsServer;
ESP8266WebServer server(6969);//listens for incoming tcp traffic

const String html1 =" <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /> </head>"
  " <img src=\"http://www.bravilor.com/Images/bravilor_logo_header.png\"><br>"
  " <h1>DILLINATOR 5000<br>WIFI Coffee Brewmaster Machine</h1><body><pre style=\"word-wrap: break-word; white-space: pre-wrap;\"></pre></body><br/>";
const String html3 =" <big><b> COFFEE IS BREWING</b></big><br/><br/>";
const String html4 =" <big>  Machine is <b>OFF</b> </big><br/><br/>";
const String html5 ="<a href=\"relay1\"><button style=\"display: block;\"><big> ON/OFF </big> </button> </a> <br/><p>all complaints of shitty coffee can be directed toward MSG Dilyard</p></html>";
//=============================================================
// Set client WiFi credentials
#define WIFI_SSID "yourAP"
#define WIFI_PASS "yourPW"
//===============================================================
//AP config
//const String APssid = "Dillinator";
//const String APpswd = "1234567890";
//int chan = 10;
//int hidden = false;
//int max_con = 4;
//=========================================================
void setup() {
  //-----outputs-----------------
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, relayState); //pin start state
  delay(10);
  //-----------------------------
  Serial.begin(115200);
  delay(100);
  //------sta setup----------------
  WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA dual mode
  //WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
  //WiFi.softAP("DNSServer CaptivePortal example xxxxx");
  //WiFi.softAP(const char* ssid, const char* password, int channel, int ssid_hidden, int max_connection)
  //WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
  //-----connect to WiFi station-------------------
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println();
  //------dns server--------------------
  // if DNSServer is started with "*" for domain name, it will reply with
  // provided IP to all DNS request
  //dnsServer.start(DNS_PORT, "*", apIP);//port 53 
  //delay(100);
  // replay to all requests with same HTML
  //server.onNotFound(htmlOutput);
  server.on("/", htmlOutput);
  //relay1
  server.on("/relay1", switchprog);
  //start webserver
  server.begin();
  Serial.println("server turned on");

}

void loop() {
  //dnsServer.processNextRequest();
  server.handleClient();
}

//==sub functions=================================
void htmlOutput() { //html page builder
  String htmlRes = html1;
  if (relayState == LOW) {
    htmlRes += html4;
  }
  else {
    htmlRes += html3;
  }
  htmlRes += html5;//add buttons
  //htmlRes += HtmlHtmlClose;//close html
  server.send(200, "text/html", htmlRes);//send string to browser
}
//======================================================
void switchprog() {
  if (relayState == LOW) {
    Serial.println("pin 2 is HIGH");
    digitalWrite(relayPin, HIGH); //LED off
    relayState = HIGH;
  }
  else {
    Serial.println("pin 2 is LOW");
    digitalWrite(relayPin, LOW);
    relayState = LOW;
  }
  htmlOutput();//html output
}
//================================================
