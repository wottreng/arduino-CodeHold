#pragma once

#include <Arduino.h>
#include <string.h>

void myWeatherInit(bool debugInput);

void myWeatherLoop1s();

void myWeatherLoop60s();

String getWindDataHTML();

String getWeatherDataJSON();

void updateWeather_EverySecond();

void calcWeather();

String getRainDataHTML();

float get_wind_speed();

String get_cardinal_dir(int dir);

int get_wind_direction();

