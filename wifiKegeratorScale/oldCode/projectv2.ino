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
#define BLYNK_PRINT Serial

#include <SPI.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

// connect to wifi and app
//  Auth Token in the Blynk App.
char auth[] = "1561561";
//  WiFi credentials.
char ssid[] = "yourAP";
char pass[] = "156156";
/* Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V1);

BLYNK_WRITE(V1)
{

  // if you type "Marco" into Terminal Widget - it will respond: "Polo:"
  if (String("Marco") == param.asStr()) {
    terminal.println("You said: 'Marco'") ;
    terminal.println("I said: 'Polo'") ;
  } else {

    // Send it back
    terminal.print("You said:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }

  // Ensure everything is sent
  terminal.flush();
}
*/

//Software Serial on Uno
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(2, 3); // RX, TX

// Your ESP8266 baud rate:
#define ESP8266_BAUD 9600

ESP8266 wifi(&EspSerial);



void setup()
{
  // Debug console
  Serial.begin(9600);// start debug computer serial

  delay(10);

  //begin ESP8266 baud rate
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  Blynk.begin(auth, wifi, ssid, pass);

  //Blynk.notify("connected");//send notification
  //terminal.clear();
  /*
  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println(F("Type 'Marco' and get a reply, or type"));
  terminal.println(F("anything else and get it printed back."));
  terminal.flush();
  */
}

void loop()
{
  Blynk.run();
  delay(100);
  float output1 = tempread(); 
  //Serial.println(output1);
  delay(2000);
  float output2 = tempread();
  //Serial.println(output2);
  int outputf = (output1+output2)/2;
  //Serial.println(outputf);
  Blynk.virtualWrite(V1,outputf);
  delay(10000);
}

float tempread ()
{
  int val = analogRead(1);
  //Serial.print(val);
  //Serial.println(" from sensor");
  float mv = val*500/1023; //celcius
  //Serial.println(mv);
  float temp = (mv*1.8)+32; // F
  //Serial.print(temp);
  //Serial.println(" to phone");
  return temp;
}
