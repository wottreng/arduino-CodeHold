#pragma once

#include <Arduino.h>
#include <string.h>


void myTime_init();

void myTime_update_time_client();

String myTime_return_current_time();

void update_time_client_status(bool current_status);

bool return_time_client_status();