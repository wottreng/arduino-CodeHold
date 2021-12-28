#include <Arduino.h>
#include <string.h>

// custom libs
#include "myScheduler.h"
#include <myTemp.h>
#include <myWifi.h>

#include <myRelay.h>
#include <myTime.h>
#include <myHistory.h>

// local vars
unsigned long last_check_60s = 0;
unsigned long last_check_20s = 0;
unsigned long last_check_1s = 0;
unsigned long last_check_5min = 0;
unsigned long last_check_10min = 0;

// tasks to complete every 20 seconds
void mySchedule_1s()
{
    if (millis() - last_check_1s >= 1000)
    {
        last_check_1s = millis();
        
    }
}

// tasks to complete every 60 seconds
void mySchedule_60s()
{
    if (millis() - last_check_60s >= 60000)
    {
        last_check_60s = millis();
        myTime_update_time_client();
    }
}

// tasks to complete every 20 seconds
void mySchedule_20s()
{
    if (millis() - last_check_20s >= 20000)
    {
        last_check_20s = millis();
        myTemp_loop();
        myRelay_loop();
    }
}

// tasks to complete every 5 min
void mySchedule_5min()
{
    if (millis() - last_check_5min >= 300000)
    {
        last_check_5min = millis();
        myWifi_connectWifi();
        myHistory_update_history();
    }
}

// tasks to complete every 10 min
void mySchedule_10min()
{
    if (millis() - last_check_5min >= 600000)
    {
        last_check_10min = millis();
        //myHistory_update_history();
    }
}