#pragma once

#include <Arduino.h>
#include <string.h>

float myTemp_return_water_temp();

float myTemp_return_air_temp();

float myTemp_return_air_humidity();

float myTemp_return_heatIndex();

void myTemp_init();

void myTemp_loop();

float myTemp_return_onTemp();
float myTemp_return_offTemp();

bool myTemp_return_tempReadError();

void myTemp_change_onOffTemp(float newOnTemp, float newOffTemp);

String myTemp_return_JSON_data();
