#include <Arduino.h>
#include <string.h>

//
#include "myRelay.h"
#include <myTemp.h>

// local vars
#define relayPin 5 // D1
#define LEDpin 16
const static bool DEBUG = false;


void myRelay_init()
{
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); //pin start state, HIGH = ON
}

//control relay
void manageRelay()
{
  float onTemp = myTemp_return_onTemp();
  float offTemp = myTemp_return_offTemp();
  float currentTemp = myTemp_return_air_temp();
  if (!myTemp_return_tempReadError())
  {
      if (currentTemp < onTemp) {
          // turn relay on
          digitalWrite(relayPin, HIGH); // on
          if (DEBUG)
              digitalWrite(LEDpin, LOW); // ON
      }
    if (currentTemp > offTemp)
    {
        // turn relay off
        digitalWrite(relayPin, LOW); // off
        if (DEBUG)
            digitalWrite(LEDpin, HIGH); // off
    }
  }
  else
  {
      if (DEBUG)
          digitalWrite(LEDpin, LOW); // ON
      digitalWrite(relayPin, LOW); // OFF
  }
}

// run every 20s
void myRelay_loop()
{
  manageRelay();
}

//
uint8_t myRelay_return_relay_pin_number()
{
  return relayPin;
}

// change relay pin status
void myRelay_switchprog()
{
  if (digitalRead(relayPin) == LOW)
  {
    digitalWrite(relayPin, HIGH); //ON
    if (DEBUG) digitalWrite(LEDpin, LOW); //ON
  }
  else
  {
    digitalWrite(relayPin, LOW); //OFF
    if (DEBUG)  digitalWrite(LEDpin, HIGH); //OFF
  }
}

String myRelay_return_JSON_data(){
  return  "\"relayPinStatus\":\"" +  String(digitalRead(relayPin)) + "\","

  ;
}
