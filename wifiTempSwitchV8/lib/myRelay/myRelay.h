#pragma once

#include <Arduino.h>
#include <string.h>


void myRelay_init();

void myRelay_loop();

uint8_t myRelay_return_relay_pin_number();

void myRelay_switchprog();

String myRelay_return_JSON_data();
