#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/*
  wifi temperature sensor post to server

   fix millis issue where after 45 days millis rolls to 0

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

//const byte tempPin = 12;  //relay pin
//byte relayState = LOW;     //pin status logic
const byte LEDpin = 16;    //onboard LED pin
byte LEDstate = HIGH;      //pin status: LOW=ON
const byte maxTry = 50;    //try this many times to connect
byte x = 0; //counter
// TEMP SENSOR STUFF
//GPIO where the DS18B20 is connected to
const byte oneWireBus = 4; //D2 on board
float ds18Temp = 0.0;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
//const byte DNS_PORT = 53;
IPAddress apIP(10, 42, 1, 1);
//DNSServer dnsServer;
ESP8266WebServer server(80);  //listens for incoming tcp traffic

//refresh: 5 //to refresh page every 5 sec
//=============================================================
// Set client WiFi credentials **needs eeprom write/read**
String WIFI_SSID;
String WIFI_PASS;
//===============================================================
//AP config
const String APssid = "wifiLakeTempV1";
const String APpswd = "1234567890";
int chan = 1;
int hidden = false;
int max_con = 4;
//EEPROM setup
byte ssidLength;
byte passLength;
//time
unsigned long previousMillis = 0;
//=========================================================
void setup() {
  //-----outputs-----------------
  pinMode(LEDpin, OUTPUT);
  //digitalWrite(tempPin, relayState);  //pin start state
  digitalWrite(LEDpin, LEDstate);      //LED pin start state, start off
  delay(10);
  //-------serial--------------------------------
  Serial.begin(9600);
  delay(100);
  //------sta setup---------------------------------
  WiFi.mode(WIFI_AP_STA);                                      //WIFI_AP_STA dual mode
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));  //#
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
  //start ds18b20 sensor
  delay(100);
  sensors.begin();
  //-----connect to WiFi station-------------------
  //try to connect to wifi based on eeprom read cred
  delay(100);
  connectWifi();  
}
//==================================================
void loop() {
  //unsigned long currentMillis = millis();
  MDNS.update();
  server.handleClient();
}
//===================================================
//==sub functions=================================
//===main page builder=========================
void mainPageHTMLpageBuilder() {
  checkTemp();
  String htmlRes = "======================================</br>";
  htmlRes += " <html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" /><meta http-equiv=\"refresh\" content=\"20; url='/'\"/></head>"  //<meta http-equiv=\"refresh\" content=\"5; url='/'\"/>
             "<script type=\"text/JavaScript\"> function timeRefresh(timeoutPeriod) {setTimeout(\"location.reload(true);\", timeoutPeriod);} </script>"
             " <h1>ESP8266 NodeMCU</h1><h3>Temp Monitor edition</h3><body><pre style=\"word-wrap: break-word;white-space: pre-wrap;\"></pre></body>"  //onLoad=\"JavaScript:timeRefresh(12000);\"
             "DS18B20 output:</br>Temp: "
             + String(ds18Temp, 2) + "F</br>";
  htmlRes += "--------------------------</br>";
  byte conStatus = WiFi.status();
  if (conStatus == 0) {
    htmlRes += "Wifi connection is Idle</br>";
  } else if (conStatus == 3) {  //connected
    htmlRes += "Connected to WiFi</br> Connected to AP:&nbsp <b>" + WIFI_SSID + "</b></br>local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>";
  } else if (conStatus == 1) {  //not connected
    htmlRes += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID + "</b></br>";
  } else {
    htmlRes += "<b>Error</b>";
  }
  //WL_IDLE_STATUS      = 0,WL_NO_SSID_AVAIL    = 1,WL_SCAN_COMPLETED   = 2,WL_CONNECTED        = 3,
  //WL_CONNECT_FAILED   = 4,WL_CONNECTION_LOST  = 5,WL_DISCONNECTED     = 6

  htmlRes += "</br>======================================</br>";
  //add buttons and options
  htmlRes += "<br/><p>to change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a>"
             "</br>wifi device can be directly connected to via wifi AP: password: 1234567890</p><p>Version 1.0</p></html>";
  //send html
  server.send(200, "text/html", htmlRes);  //send string to browser
}

//=====/wifi AP setup page===========================================
void wifiCredentialUpdatePage() {
  String setuphtml = "<!DOCTYPE HTML><html><head>"
                     "<title>ESP Input Form</title>"
                     "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                     "</head><body><p>current wifi AP SSID: &nbsp <b>";
  setuphtml += WIFI_SSID;
  setuphtml += "</b></p>"
               "<form action=\"/input\" method=\"POST\">"
               " Wifi ssid :<br>"
               " <input type=\"text\" name=\"wifi SSID\" value=\"enter wifi name\">"
               " <br>"
               " Wifi password:<br>"
               " <input type=\"password\" name=\"password\" value=\"enter wifi password\">"
               " <br><br>"
               " <input type=\"submit\" value=\"Submit\">"
               "</form> "
               "</body></html>";
  String htmlConfig = setuphtml;
  server.send(200, "text/html", htmlConfig);  //send string to browser
}
//------------------------------------------------
// This routine is executed when you change wifi AP
void APinputChange() {
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
  EEPROM.commit();  //flush write content to eeprom

  String s = "<meta http-equiv=\"refresh\" content=\"30; url='/'\" />"
             "<body> trying to connect to new wifi connection </br> "
             "if connection is successful you will be disconnected from soft AP</br>"
             "please wait a minute then click below to go to main page</body>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s);  //Send web page
  //delay(200);
  //server.stop();
  delay(50);
  connectWifi();
}
//==========================================================
//===connect to wifi function==========================
void connectWifi() {
  //Serial.print("Connecting to ");
  //WiFi.setPhyMode(WIFI_PHY_MODE_11B);//11B,11G,11N B=range, N=Mbps
  //Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  x = 0;
  while (x < maxTry) { //50
    //server.handleClient();
    delay(20);
    //Serial.print(".");
    x += 1;
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(WIFI_SSID);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      //WiFi.mode(WIFI_STA);  //WIFI_AP_STA dual mode
      x = 100;
      //
      delay(20);
      // Set up mDNS responder:
      // - first argument is the domain name, in this example
      //   the fully-qualified domain name is "esp8266.local"
      // - second argument is the IP address to advertise
      //   we send our IP address on the WiFi network
      MDNS.begin("lakeTemp"); //lakeTemp.local
    }
  }
  if (x == (maxTry)) {
    Serial.print("wifi not connected");
    blinkLED(); //blink onboard LED
  }
  //delay(50);
  //------server--------------------
  //server.stop();
  server.onNotFound(notFound);  // after ip address /
  server.on("/", HTTP_GET, mainPageHTMLpageBuilder);
  server.on("/temp", HTTP_GET, tempData);
  server.on("/APsetup", HTTP_GET, wifiCredentialUpdatePage); //see current AP setup
  server.on("/input", HTTP_POST, APinputChange);  //change AP setup
  server.begin();
  //Serial.println("server turned on");
}
//==========================================
//blink onboard LED
void blinkLED() {
  //WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA dual mode
  x = 0;
  while (x < 10) {
    LEDstate = !LEDstate;
    digitalWrite(LEDpin, LEDstate);
    delay(100);
    x += 1;
  }
}
//=====================================
void notFound() {  //when stuff after / is incorrect
  String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
             "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  server.send(200, "text/html", s);  //Send web page
}

// ds18b20 function
void checkTemp() {
  sensors.requestTemperatures();
  //float temperatureC = sensors.getTempCByIndex(0);
  ds18Temp = sensors.getTempFByIndex(0);
}

void tempData(){
  checkTemp();
  String output = "{\"LakeTemp\":\"" + String(ds18Temp, 2) +"\"}";
  server.send(200, "application/json", output); 
}
/*
void sendData(String dataX) {
  String message = "uploadData="+dataX;
  String messageLength = String(message.length());
  //Your Domain name with URL path or IP address with path
  WiFiClientSecure httpsClient;
  httpsClient.setFingerprint(fingerprint);
  httpsClient.setTimeout(15000);  // 15 Seconds
  //Serial.print("HTTPS Connecting");
  int r = 0;  //retry counter
  while ((!httpsClient.connect(host, httpsPort)) && (r < 30)) {
    delay(100);
    //Serial.print(".");
    r++;
  }
  if (r == 30) {
    //Serial.println("Connection failed");
    delay(10);
  } else {
    //Serial.println("Connected to web");
    //POST Data
    String Link = "/19sdf8g31h";
    httpsClient.print(String("POST ") + Link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + 
      "Content-Type: application/x-www-form-urlencoded" + "\r\n" + "Content-Length: " + messageLength +
      "\r\n\r\n" + message + "\r\n" + "Connection: close\r\n\r\n");
  }
  
  //Serial.println("request sent");
  /*
  while (httpsClient.connected()) {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  
  Serial.println("reply was:");
  Serial.println("==========");
  String line;
  while (httpsClient.available()) {
    line = httpsClient.readStringUntil('\n');  //Read Line by Line
    Serial.println(line);                      //Print response
  }
  Serial.println("==========");
  Serial.println("closing connection");
  */
//}

/*
void deepSleep(){
   ESP.deepSleep(10E6); //10 sec, maximum timed Deep Sleep interval ~ 3 to 4 hours depending on the RTC timer
   // gpio 16(D0) needs to be connected to rst pin
}
*/
