#include <Arduino.h>
#include <string.h>
#include <DHT.h>

// custom libs
#include "myTemp.h"
#include <myEEPROM.h>
#include <myRelay.h>

const static bool DEBUG = false;

// local variables ======================================

#define LEDpin 16

// TEMP SENSOR Relay
float onTemp = 0.0;
float offTemp = 1.0;

// EEPROM
const uint8_t onTempLoc = 100;
const uint8_t offTempLoc = 120;

//float water_temp_history_24h[24];

// DHT sensor -------------
#define DHTPIN 13 // D7
#define DHTTYPE DHT22 // 22:white, 11:blue sensor
DHT dht(DHTPIN, DHTTYPE);

bool tempReadError = false;
float dhtTemp = 0.0;
float dhtHumid = 0.0;
float heatIndex = 0.0;
//float air_temp_history_24h[24];

// indexs
//byte temp_history_index_60m = 0;

// last check timers
//unsigned long temp_Loop_60s_last_check = 0;
//unsigned long temp_Loop_60m_last_check = 0;

// start sensors and read temp values from eeprom
void myTemp_init()
{
    // dht sensor
    dht.begin();

    //
    String rawOnTemp = read_eeprom(onTempLoc);
    String rawOffTemp = read_eeprom(offTempLoc);
    onTemp = rawOnTemp.toFloat();
    offTemp = rawOffTemp.toFloat();
}



/*
void myTemp_Loop_60m(){
        if ((millis() - temp_Loop_60m_last_check) > 3600000) {
        update_temp_history();
        temp_Loop_60m_last_check = millis();
    }
}
*/

// return onTemp
float myTemp_return_onTemp(){
    return onTemp;
}
//
float myTemp_return_offTemp(){
    return offTemp;
}

float myTemp_return_heatIndex(){
    return heatIndex;
}

// return air temp from dht11 sensor
float myTemp_return_air_temp(){
    return dhtTemp;
}

// air humidity from dht11 sensor
float myTemp_return_air_humidity(){
    return dhtHumid;
}

bool myTemp_return_tempReadError(){
    return tempReadError;
}

// DHT22 function
void myTemp_check_air_temp_humidity_DHT11()
{
    float t = dht.readTemperature(true);
    float h = dht.readHumidity();

    if (isnan(h) || isnan(t)) {
        // bad reading
        tempReadError = true;
    } else {
        dhtHumid = h;
        dhtTemp = t;
        heatIndex = dht.computeHeatIndex();
        tempReadError = false;
    }
}

// change ontemp, offtemp
void myTemp_change_onOffTemp(float newOnTemp, float newOffTemp){

    if(newOnTemp > newOffTemp){    
    onTemp = 1.0;
    offTemp = 2.0;
  } else{
    onTemp = newOnTemp;
    offTemp = newOffTemp;
    writeToEEPROM(String(onTemp), onTempLoc);
    writeToEEPROM(String(offTemp), offTempLoc);
  }
}

// main loop task every 20 seconds, run in scheduler
void myTemp_loop()
{
    myTemp_check_air_temp_humidity_DHT11();
}

String myTemp_return_JSON_data(){
    return "\"dhtTemp\":\"" + String(dhtTemp, 2) + "\","
        "\"dhtHumid\":\""  + String(dhtHumid, 2) + "\","
        "\"heatIndex\":\""  + String(heatIndex, 2) + "\","
        "\"onTemp\":\""  + String(onTemp, 2) + "\","
        "\"offTemp\":\""  + String(offTemp, 2) + "\","
        "\"tempReadError\":\""  + String(tempReadError) + "\","
        ;
}