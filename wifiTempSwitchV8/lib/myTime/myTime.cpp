#include <Arduino.h>
#include <string.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

// custom libs
#include "myTime.h"

// VERSION 1.1

// local vars --------------
bool time_client_active = false; // keep track of wifi connection
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", -18000, 60000);

// functions =============================

// initialize time client, need wifi connection
void myTime_init(){
    timeClient.begin();
    update_time_client_status(true);
}

// run in main loop, if connected to wifi
void myTime_update_time_client(){
    if(time_client_active == true)  timeClient.update();
}

// returns time hours and minutes, ex. "12:34"
String myTime_return_current_time(){
    if(time_client_active == true)  return (timeClient.getFormattedTime()).substring(0, 5); 
    
    else return "n/a";
}

void update_time_client_status(bool current_status){
    time_client_active = current_status;
}

bool return_time_client_status(){
    return time_client_active;
}