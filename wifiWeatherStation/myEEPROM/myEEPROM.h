#pragma once


void EEPROMinit();

String read_eeprom(int address);

void writeToEEPROM(String data, int address);
