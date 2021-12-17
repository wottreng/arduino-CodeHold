#include <Arduino.h>
#include <string.h>
#include <EEPROM.h>

// custom lib
#include "myEEPROM.h"


//initialize eeprom
void EEPROMinit(){
     EEPROM.begin(512);//eeprom is 512bytes in size
}

// EEPROM commands =====================================================
void writeToEEPROM(String data,int address){
  byte dataLength = data.length();
  char charInput[dataLength + 1];
  // string to char array
  strcpy(charInput, data.c_str());
  // write to EEPROM
  EEPROM.write(address, dataLength);
  for (int i = 0; i < dataLength; i++) {
    EEPROM.write(address + 1 + i, charInput[i]);
  }
  EEPROM.commit();//flush, write content to eeprom
}

// read eeprom
String read_eeprom(int address) {
  byte dataLength = EEPROM.read(address);
  String data;
  byte a;
    if(dataLength > 20){
      a = 0;
      data += char(a);
  }else{
    for (int i = 0; i < dataLength; i++) {
      a = EEPROM.read(address + 1 + i);
      data += char(a);
    }
  }
  return data;
  }











