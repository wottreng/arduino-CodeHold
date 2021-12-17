#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <string.h>

// custom libs
#include "myTemp.h"


extern bool debug;

// local variables ======================================

float ds18Temp = 0.0;

// ds18b20 pin / one wire bus pin
const byte oneWireBus = 4; // D2 on board

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// last check timers
unsigned long tempLoop20sLastCheck = 0;

// start ds18b20 sensor
void myTempInit(bool debugInput = false)
{
    if(debugInput)
        debug = true;
    sensors.begin();
}

// 20 second loop timer
void myTempLoop20s()
{
    if ((millis() - tempLoop20sLastCheck) > 20000) {
        checkDS18B20();
        tempLoop20sLastCheck = millis();
    }
}

// return ds18b20 temp data
float returnTemp()
{
    return ds18Temp;
}

// return ds18b20 temp data as string
String returnTempString(){
    return String(ds18Temp);
}

// ds18b20 sensor 
void checkDS18B20()
{
    sensors.requestTemperatures();
    // float temperatureC = sensors.getTempCByIndex(0);
    ds18Temp = sensors.getTempFByIndex(0);
}
