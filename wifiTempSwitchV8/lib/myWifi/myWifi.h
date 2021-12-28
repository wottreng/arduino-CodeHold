#pragma once

#include <Arduino.h>
#include <string.h>

void myWifi_Init(bool debugInput);

void myWifi_Loop();

void myWifi_connectWifi(bool New_Connection = false);

void myWifi_web_server_config();

uint8_t myWifi_return_connection_status();