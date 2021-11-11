#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266HTTPUpdateServer.h>
#include <math.h>

/*
  weather and lake temp sensor for dock up north
  
   pinMode(, OUTPUT/INPUT);pin# 1,3,15,13,12,14,2,0,4,5,16,9,10
   ADC0: analogRead(A0)
   interupt pins: D0-D8 = pin 16,5,4,

  weather station wiring:
  4 wire harnes:
    Winddir:
      black =>(jumper to A0) => 1k resistor => ground
      green => 3.3v
    windspeed:
      red => ground, 0v
      yellow => D5

  2 wire harness - rain bucket
  red => ground, 0v
  green => D6
*/
String version = "2.1";
const bool debug = false;

//const byte tempPin = 12;  //relay pin
//byte relayState = LOW;     //pin status logic
const byte LEDpin = 16;    //onboard LED pin
// TEMP SENSOR STUFF  ------
//GPIO where the DS18B20 is connected to
const byte oneWireBus = 4; //D2 on board
float ds18Temp = 0.0;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);
//=======
IPAddress apIP(10, 42, 2, 1);
ESP8266WebServer httpServer(80);  //listens for incoming tcp traffic
ESP8266HTTPUpdateServer httpUpdater;
const String hostName = "weather";
//============
String WIFI_SSID;
String WIFI_PASS;
// == EEPROM ==
const byte wifiSSIDloc  = 0;
const byte wifiPSWDloc = 20;
//======
//AP config
const String APssid = "wifiLakeInfo";
const String APpswd = "123456789";
int chan = 1;
int hidden = false;
int max_con = 4;
const byte maxTry = 10;    //try this many times to connect
//time
unsigned long oldtime = 0;
// weather vars -------------
const byte windspeedPin = 14; //interupt, D5
const byte rainBucketPin = 12; //interupt, D6
const byte windDirPin = A0;

long lastSecond; //The millis counter to see when a second rolls by
byte seconds; //When it hits 60, increase the current minute
byte seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes; //Keeps track of where we are in various arrays of data
byte minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data

long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;

//We need to keep track of the following variables:
//Wind speed/dir each update (no storage)
//Wind gust/dir over the day (no storage)
//Wind speed/dir, avg over 2 minutes (store 1 per second)
//Wind gust/dir over last 10 minutes (store 1 per minute)
//Rain over the past hour (store 1 per minute)
//Total rain over date (store one per day)

byte windspdavg[120]; //120 bytes to keep track of 2 minute average

#define WIND_DIR_AVG_SIZE 120
int winddiravg[WIND_DIR_AVG_SIZE]; //120 ints to keep track of 2 minute average
float windgust_10m[10]; //10 floats to keep track of 10 minute max
int windgustdirection_10m[10]; //10 ints to keep track of 10 minute max
volatile float rainHour[60]; //60 floating numbers to keep track of 60 minutes of rain

//These are all the weather values that wunderground expects:
int winddir = 0; // [0-360 instantaneous wind direction]
float windspeedmph = 0; // [mph instantaneous wind speed]
float windgustmph = 0; // [mph current wind gust, using software specific time period]
int windgustdir = 0; // [0-360 using software specific time period]
float windspdmph_avg2m = 0; // [mph 2 minute average wind speed mph]
int winddir_avg2m = 0; // [0-360 2 minute average wind direction]
float windgustmph_10m = 0; // [mph past 10 minutes wind gust mph ]
int windgustdir_10m = 0; // [0-360 past 10 minutes wind gust direction]
float humidity = 0; // [%]
float tempf = 0; // [temperature F]
float rainin = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
volatile float dailyrainin = 0; // [rain inches so far today in local time]
//float baromin = 30.03;// [barom in] - It's hard to calculate baromin locally, do this in the agent
//float pressure = 0;
//float dewptf; // [dewpoint F] - It's hard to calculate dewpoint locally, do this in the agent

float batt_lvl = 11.8; //[analog value from 0 to 1023]
float light_lvl = 455; //[analog value from 0 to 1023]

// volatiles are subject to modification by IRQs
volatile unsigned long raintime, rainlast, raininterval, rain;
//

//=========================================================
void setup() {
  //-----outputs-----------------
  pinMode(LEDpin, OUTPUT);
  pinMode(windspeedPin, INPUT_PULLUP);
  pinMode(rainBucketPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rainBucketPin), rainIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(windspeedPin), wspeedIRQ, FALLING);
  digitalWrite(LEDpin, HIGH); //LED pin start state, start off, LOW = on
  delay(10);
  //
  seconds = 0;
  lastSecond = millis();    
  //-------serial------
  if(debug){
    Serial.begin(115200);
    delay(100);
  }  
  //read wifi creds from eeprom---------------------
  EEPROM.begin(512);//eeprom is 512bytes in size
  delay(10);
  WIFI_SSID = read_eeprom(wifiSSIDloc); //string
  delay(10);
  WIFI_PASS = read_eeprom(wifiPSWDloc);
  delay(10);
  //start ds18b20 sensor
  delay(100);
  sensors.begin();
  //  
  delay(100);
  connectWifi();  
}
//==================================================
void loop() {
  //unsigned long currentMillis = millis();
  httpServer.handleClient();
  MDNS.update();
  if ((millis()-oldtime)>20000) {
    checkDS18B20();
    calcWeather();
    oldtime = millis();
    if(debug){
      Serial.println("wifi status: "+ String(WiFi.status())); //3=connected
      Serial.println("ds18b20 temp: "+ String(ds18Temp));
      printWeather();
    }
  }
  if (millis() - lastSecond >= 1000){ //must be every second
    updateWeather();
  }
}
//==sub functions=================================
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
  httpServer.send(200, "text/html", htmlConfig);  //send string to browser
}
//------------------------------------------------
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
//==========================================================
//===connect to wifi function==========================
void connectWifi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(hostName.c_str());
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  byte x = 0;
  while (x <= maxTry) {
    delay(800);
    if(debug){
      Serial.print(".");
    }    
    x += 1;
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.mode(WIFI_STA);  //WIFI_AP_STA dual mode
        x = 100;
        digitalWrite(LEDpin, HIGH); //off
        if(debug){
          Serial.println("");
          Serial.print("Connected to ");
          Serial.println(WIFI_SSID);
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
        }
    }
    if (WiFi.status() != WL_CONNECTED && x == maxTry) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));//#
        WiFi.softAP(APssid, APpswd, chan, hidden, max_con);
        digitalWrite(LEDpin, LOW); //on
        if(debug){
          Serial.println("no AP connection, starting hotspot");
        }
    }
  }
  MDNS.begin(hostName.c_str());

  //------server--------------------
  //server.stop();
  httpServer.onNotFound(notFound);  // after ip address /
  httpServer.on("/", HTTP_GET, mainPageHTMLpageBuilder);
  httpServer.on("/APsetup", HTTP_GET, wifiCredentialUpdatePage); //see current AP setup
  httpServer.on("/input", HTTP_POST, APinputChange);  //change AP setup
  httpServer.on("/api0", HTTP_GET, jsonData);
  
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  if(debug){
    Serial.println("Server started");
  }
}
//=====================================
void notFound() {  //when stuff after / is incorrect
  String s = "<meta http-equiv=\"refresh\" content=\"5; url='/'\" />"
             "<body> not a know page for ESP server </br> you will be directed automaticly or click button below to be redirected to main page</br>"
             "<br/><p>click to go back to main page:&nbsp <a href=\"/\"><button style=\"display: block;\">AP Main Page</button></a>";
  httpServer.send(200, "text/html", s);  //Send web page
}

// ds18b20 function
void checkDS18B20() {
  sensors.requestTemperatures();
  //float temperatureC = sensors.getTempCByIndex(0);
  ds18Temp = sensors.getTempFByIndex(0);
}

void jsonData(){
  String output = "{";
  output += "\"LakeTempF\":\"" + String(ds18Temp, 2) +"\",";
  output += "\"windDirection\":\"" + String(winddir) +"\",";
  output += "\"winddir_avg2m\":\"" + String(winddir_avg2m) +"\",";
  output += "\"windspeedmph\":\"" + String(windspeedmph, 2) +"\",";
  output += "\"windspdmph_avg2m\":\"" + String(windspdmph_avg2m, 2) +"\",";
  output += "\"windgustmph\":\"" + String(windgustmph, 2) +"\",";
  output += "\"windgustdirection\":\"" + String(windgustdir) +"\",";  
  output += "\"windgustmph_10m\":\"" + String(windgustmph_10m, 2) +"\",";
  output += "\"windgustdir_10m\":\"" + String(windgustdir_10m) +"\",";
  output += "\"rainInches\":\"" + String(rainin, 2) +"\",";
  output += "\"dailyRainInches\":\"" + String(dailyrainin, 2) +"\",";  
   if (digitalRead(LEDpin)){
    // 1 -> off
    output += "\"LED\":\"off\"";
  }else{
    // 0 -> on
    output += "\"LED\":\"on\"";
  }
  output += "}";

  httpServer.send(200, "application/json", output); 
}
// weather functions =================================================
void updateWeather(){
    lastSecond += 1000;

    //Take a speed and direction reading every second for 2 minute average
    if (++seconds_2m > 119) seconds_2m = 0;

    //Calc the wind speed and direction every second for 120 second to get 2 minute average
    float currentSpeed = get_wind_speed();
    windspeedmph = currentSpeed;//update global variable for windspeed when using the printWeather() function
    //float currentSpeed = random(5); //For testing
    int currentDirection = get_wind_direction();
    windspdavg[seconds_2m] = (int)currentSpeed;
    winddiravg[seconds_2m] = currentDirection;
    //if(seconds_2m % 10 == 0) displayArrays(); //For testing

    //Check to see if this is a gust for the minute
    if (currentSpeed > windgust_10m[minutes_10m])
    {
      windgust_10m[minutes_10m] = currentSpeed;
      windgustdirection_10m[minutes_10m] = currentDirection;
    }

    //Check to see if this is a gust for the day
    if (currentSpeed > windgustmph)
    {
      windgustmph = currentSpeed;
      windgustdir = currentDirection;
    }

    if (++seconds > 59)
    {
      seconds = 0;

      if (++minutes > 59) minutes = 0;
      if (++minutes_10m > 9) minutes_10m = 0;

      rainHour[minutes] = 0; //Zero out this minute's rainfall amount
      windgust_10m[minutes_10m] = 0; //Zero out this minute's gust
    }  

    //digitalWrite(STAT1, LOW); //Turn off stat LED
}

//--------------------------
float get_wind_speed()
{
  float deltaTime = millis() - lastWindCheck; //750ms

  deltaTime /= 1000.0; //Covert to seconds

  float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

  windClicks = 0; //Reset and start watching for new wind
  lastWindCheck = millis();

  windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

  /* Serial.println();
    Serial.print("Windspeed:");
    Serial.println(windSpeed);
  */

  return (windSpeed);
}
// ---------------------
int get_wind_direction(){
  unsigned int adc;
  adc = analogRead(windDirPin); // get the current reading from the sensor
  /*if(debug){
    Serial.println("wind dir: "+String(adc));
  }
  */

  // NEED TO DO MEASUREMENTS
  // ADD 1K resitor inline, try 3.3 volts input
  // N: 33, NE:117, E:545, SE:341, S: 223, SW:65, W:10, NW: 19  
  if (adc < 12) return (270); //W  10
  if (adc < 23) return (315); //NW  19
  if (adc < 35) return (0); //N  33
  if (adc < 70) return (225); //SW  65
  if (adc < 125) return (45); //NE  117
  if (adc < 245) return (180); //S 223
  if (adc < 360) return (135); //SE 341 
  if (adc < 650) return (90); // E 545
  
  return (-1); // error, disconnected?
}
// ----------------
void calcWeather(){
  //Calc winddir
  winddir = get_wind_direction();

  //Calc windspeed
  //windspeedmph = get_wind_speed(); //This is calculated in the main loop 

  //Calc windgustmph
  //Calc windgustdir
  //These are calculated in the main loop

  //Calc windspdmph_avg2m
  float temp = 0;
  for (int i = 0 ; i < 120 ; i++)
    temp += windspdavg[i];
  temp /= 120.0;
  windspdmph_avg2m = temp;

  //Calc winddir_avg2m, Wind Direction
  //You can't just take the average. Google "mean of circular quantities" for more info
  //We will use the Mitsuta method because it doesn't require trig functions
  //And because it sounds cool.
  //Based on: http://abelian.org/vlf/bearings.html
  //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
  long sum = winddiravg[0];
  int D = winddiravg[0];
  for (int i = 1 ; i < WIND_DIR_AVG_SIZE ; i++)
  {
    int delta = winddiravg[i] - D;

    if (delta < -180)
      D += delta + 360;
    else if (delta > 180)
      D += delta - 360;
    else
      D += delta;

    sum += D;
  }
  winddir_avg2m = sum / WIND_DIR_AVG_SIZE;
  if (winddir_avg2m >= 360) winddir_avg2m -= 360;
  if (winddir_avg2m < 0) winddir_avg2m += 360;

  //Find the largest windgust in the last 10 minutes
  windgustmph_10m = 0;
  windgustdir_10m = 0;
  //Step through the 10 minutes
  for (int i = 0; i < 10 ; i++)
  {
    if (windgust_10m[i] > windgustmph_10m)
    {
      windgustmph_10m = windgust_10m[i];
      windgustdir_10m = windgustdirection_10m[i];
    }
  }

  //Total rainfall for the day is calculated within the interrupt
  //Calculate amount of rainfall for the last 60 minutes
  rainin = 0;
  for (int i = 0 ; i < 60 ; i++)
    rainin += rainHour[i];

}
// ------------------------
//Interrupt routines (these are called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
ICACHE_RAM_ATTR void rainIRQ()
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge, attached to input D2
{
  raintime = millis(); // grab current time
  raininterval = raintime - rainlast; // calculate interval between this and last event

  if (raininterval > 10) // ignore switch-bounce glitches less than 10mS after initial edge
  {
    dailyrainin += 0.011; //Each dump is 0.011" of water
    rainHour[minutes] += 0.011; //Increase this minute's amount of rain

    rainlast = raintime; // set up for next event
  }
}
ICACHE_RAM_ATTR void wspeedIRQ(){
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
  if (millis() - lastWindIRQ > 10) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
  {
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
    if(debug){
      Serial.println("click");
    }
  }
}

// -------------------------
void printWeather()
{
  Serial.println();
  Serial.print("winddir=");
  Serial.print(winddir);
  Serial.print(",windspeedmph=");
  Serial.print(windspeedmph, 1);
  Serial.print(",windgustmph=");
  Serial.print(windgustmph, 1);
  Serial.print(",windgustdir=");
  Serial.print(windgustdir);
  Serial.print(",windspdmph_avg2m=");
  Serial.print(windspdmph_avg2m, 1);
  Serial.print(",winddir_avg2m=");
  Serial.print(winddir_avg2m);
  Serial.print(",windgustmph_10m=");
  Serial.print(windgustmph_10m, 1);
  Serial.print(",windgustdir_10m=");
  Serial.print(windgustdir_10m);
  //Serial.print(",humidity=");
  //Serial.print(humidity, 1);
  //Serial.print(",tempf=");
  //Serial.print(tempf, 1);
  Serial.print(",rainin=");
  Serial.print(rainin, 2);
  Serial.print(",dailyrainin=");
  Serial.print(dailyrainin, 2);
  //Serial.print(",pressure=");
  //Serial.print(pressure, 2);
  //Serial.print(",batt_lvl=");
  //Serial.print(batt_lvl, 2);
  //Serial.print(",light_lvl=");
  //Serial.print(light_lvl, 2);
  //Serial.print(",");
  //Serial.println("#");

}


// =======================================
/*
void sendData(String dataX) {
  String message = "uploadData="+dataX+"&xkeyx=wottreng69&fileName=lakeTemp";
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
 
}
*/

//===main page builder=========================
void mainPageHTMLpageBuilder() {
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
    htmlRes += "Connected to WiFi</br> Connected to AP:&nbsp <b>" + WIFI_SSID + "</b></br>local IP:&nbsp<b>" + WiFi.localIP().toString() + "</b></br>" + "Connection Strength: <b>"+ WiFi.RSSI() + " dbm</b>";
  } else if (conStatus == 1) {  //not connected
    htmlRes += "Not connected to internet</br> Can not connect to AP:&nbsp<b>" + WIFI_SSID + "</b></br>";
  } else if (conStatus == 7){
    htmlRes += "Disconnected from access point";
  } else {
     htmlRes += "<b>Connection Status: "+ String(conStatus) +"</b></br>";
  }
  //WL_IDLE_STATUS      = 0,WL_NO_SSID_AVAIL    = 1,WL_SCAN_COMPLETED   = 2,WL_CONNECTED        = 3,
  //WL_CONNECT_FAILED   = 4,WL_CONNECTION_LOST  = 5,WL_DISCONNECTED     = 6

  htmlRes += "</br>======================================</br>";
  //add buttons and options
  htmlRes += "<br/><p>to change wifi AP: <a href=\"APsetup\"><button style=\"display: block;\">Wifi Setup</button></a>"
             "</br><p>Version "+version+"</p></html>";
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

void blinkLED() {
  //WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA dual mode
  byte x = 0;
  while (x < 6) {
    digitalWrite(LEDpin, !digitalRead(LEDpin));
    delay(100);
    x += 1;
  }
}
  /* other helpful commands
  ESP.restart(); //reboot

  void deepSleep(){
   ESP.deepSleep(10E6); //10 sec, maximum timed Deep Sleep interval ~ 3 to 4 hours depending on the RTC timer
   // gpio 16(D0) needs to be connected to rst pin
}
//blink onboard LED
void blinkLED() {
  byte x = 0;
  while (x < 10) {
    digitalWrite(LEDpin, !digitalRead(LEDpin));
    delay(1000);
    x += 1;
  }
}
  */
