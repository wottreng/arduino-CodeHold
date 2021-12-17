#pragma once

#include <Arduino.h>
#include <string.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>


void myWifiInit(bool debugInput);

void myWifiLoop();

void connectWifi();

void notFound();

void jsonData();

void mainPageHTMLpageBuilder();
