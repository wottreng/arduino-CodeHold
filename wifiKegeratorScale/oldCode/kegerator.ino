/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************

  This example shows how to use ESP8266 Shield (with AT commands)
  to connect your project to Blynk.

  WARNING!
    It's very tricky to get it working. Please read this article:
    http://help.blynk.cc/hardware-and-libraries/arduino/esp8266-with-at-firmware

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
//----------------------------------------
#include <config.h>
#include <HX711_ADC.h>
#include <HX711.h>
#include <SPI.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
//#include <EEPROM.h>//eeprom library
// connect to wifi and app
//  Auth Token in the Blynk App.
char auth[] = "128651615";
//  WiFi credentials.
char ssid[] = "yourAP";
char pass[] = "yourPW";



//Software Serial on Uno
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(2, 3); // RX, TX

//define weight scale Dout,CLK
#define DOUT  5
#define CLK  7
HX711 scale;

// ESP8266 baud rate:
#define ESP8266_BAUD 9600
ESP8266 wifi(&EspSerial);

void setup()
{
  // Debug console
 Serial.begin(9600);// start debug computer serial
  delay(10);
  //wifi module start
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
  //weight scale start
  scale.begin(DOUT, CLK);
  delay(10);
  Blynk.begin(auth, wifi, ssid, pass);
}
int x=1;// so notifications dont keep being sent
void loop()
{
  Blynk.run();
  delay(100);
  
  //------------------------------------
  // temp calculations
  int n,avg;
  float sum=0, output[10];
    for (n=0; n<10; n++) {
    output[n] = {tempread()}; 
    delay(100);
    }
    for (n=0; n<10; n++){
    sum =sum + output[n];
    }
    avg = sum/10;
   // Serial.print(avg);
   // Serial.println(" temp F");
    
  Blynk.virtualWrite(V1,avg);
  //-------------------------------
  //weight calculations
   float outputx = scale.read_average();

   
 
  //10lbs is empty, 55lbs is full
  /*int perfull = (2.1277*outputx);//percent calc
  if (perfull <0){
    perfull =0;
    }
  if (perfull<10&&x==1){
    //Blynk.notify("almost out");//send app notification
    x = 0;
  }
  if (perfull>100){
    perfull=100;
  }
  */
  //Serial.print(outputx);
  //Serial.println(" scale output");
  Blynk.virtualWrite(V2,outputx);
  delay(10000);//wait 10 sec
}
//functions------------------------------
float tempread (){ //read temp sensor
  int val =analogRead(1);
  float mv = val*500/1023; //celcius
  float temp = (mv*1.8)+32; // F
  return temp;
}
//-------------
float tempComp (scaleOutput){
  int scaleOutput, temp//read in temp and scale output
  int tempdif = temp - 40; //40F is baseline temp
 scaleOutput = scaleOutput*factor^tempdif //adjust scale output to comp for temp
  //output  percent full
  //40F:115820,30F:122963
  //scaleOutput = -714.38(temp)+144395 when 0% full
}

  


*/
