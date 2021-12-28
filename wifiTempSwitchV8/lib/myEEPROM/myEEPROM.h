#pragma once

#include <Arduino.h>
#include <string.h>


void EEPROMinit();

String read_eeprom(int address);

void writeToEEPROM(String data, int address);
