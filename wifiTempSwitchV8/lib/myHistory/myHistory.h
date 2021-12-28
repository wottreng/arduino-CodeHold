#pragma once

#include <Arduino.h>
#include <string.h>

String return_int_array_history(int array[], uint8_t array_length, uint8_t start_index=0);

String return_float_array_history(float array[], uint8_t array_length, uint8_t start_index=0);

String return_char_array_history(char array[][6], uint8_t array_length, uint8_t start_index=0);

String return_history_html();

void myHistory_update_history();