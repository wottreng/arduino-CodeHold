#pragma once

#include <Arduino.h>
#include <string.h>

float returnTemp();

String returnTempString();

void myTempInit(bool debugInput);

void checkDS18B20();

void myTempLoop20s();
