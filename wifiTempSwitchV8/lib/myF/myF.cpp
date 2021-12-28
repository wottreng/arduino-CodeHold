#include <Arduino.h>
#include <string.h>

// ref: https://espwifi.readthedocs.io/en/latest/libraries.html

// custom libs
#include "myF.h"

// local vars --------------

// onboard LED pin
#define LEDpin 16
//
String version = "8.0";
//
bool debug = false;

// functions =============================

// initialize LED pin
void myFinit(bool debugInput = false)
{
    if (debugInput)
        debug = true;
    if (debug)
        pinMode(LEDpin, OUTPUT);
}

// get firmware version
String getVersion()
{
    return version;
}

// get builtin LED pin number
const byte getLEDpin()
{
    return LEDpin;
}

// blink onboard LED 6 times
void blinkLED()
{
    // WiFi.mode(WIFI_AP_STA);//WIFI_AP_STA dual mode
    byte x = 0;
    while (x < 6) {
        if (debug)
            digitalWrite(LEDpin, !digitalRead(LEDpin));
        delay(300);
        x += 1;
    }
}

//
void reboot()
{
    ESP.restart();
}

void deepSleep()
{
    ESP.deepSleep(10E6); // 10 sec, maximum timed Deep Sleep interval ~ 3 to 4 hours depending on the RTC timer
    // gpio 16(D0) needs to be connected to rst pin
}
