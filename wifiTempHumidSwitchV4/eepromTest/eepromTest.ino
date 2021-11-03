/*  This code is for the EEPROM in the ESP8266;
     the EEPROM definition in ESP32, Arduino Uno, Mega differs from that of the ESP8266.
    https://pijaeducation.com/eeprom-in-arduino-and-esp/
*/

// include EEPROM Library
#include <EEPROM.h>

/* Defining data storage addresses
   As we know size of int is 4 Bytes,
    If we store int at index 0 then the next EEPROM address can be anything (4,5..10 etc.) after 4 byte address i.e. (0-3)
 You can use this statement to find size of int type data Serial.println(sizeof(int))
*/
//int eepromAddr1 = 0, eepromAddr2 = 256;

// We store int and String data in the ESP8266's EEPROM.

String stringData = "My String";

// put is used to write to EEPROM, while read is used to read from it.
void setup() {
  Serial.begin(9600);
  /* Begin with EEPROM by deciding how much EEPROM memory you want to use.
     The ESP8266's maximum EEPROM size is 4096 bytes (4 KB), but we're just using 512 bytes here.
  */
  EEPROM.begin(512);
  delay(500);

  // User's message
  Serial.println("To put String Send → 'string' ");
  Serial.println("To get data Send → 'read' ");
  Serial.println("To clear EEPROM Send → 'clear' ");
}

void loop() {
  if (Serial.available()) {
    String x = Serial.readString();
    delay(10);
    Serial.println("");
    Serial.print("Input → ");
    Serial.println(x);

    /* If the user enters 'read' as input,
        we will read from the EEPROM using the 'EEPROM get' function.
    */
    if (x == "read") {
      
      String ss = read_eeprom(200);
      Serial.print("Readed String:||");
      Serial.print(ss);
      Serial.println("||");
      delay(50);
    }

    else if (x == "string") {
      writeToEEPROM(stringData,200);
      Serial.println("Done!");
    }


    /* if user send 'clear' as input
        then we will clear data from EEPROM
    */
    else if (x == "clear") {
      for (int i = 0 ; i < 512 ; i++) {
        EEPROM.write(i, 0);
      }
      // Don't forget EEPROM.end();
      EEPROM.end();
      Serial.println("EEPROM Clear Done!");
    }
    /* if user send any other string as input
        then display a message to user
    */
    else {
      Serial.println("Wrong Input");
    }
  }
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
  //
  Serial.print("||");
  Serial.print(dataLength);
  Serial.println("||");
  //
  
  //byte dataLength = EEPROM.read(address);
  if(dataLength > 20){
    return "error";
  }
  String data = "";
  for (int i = 0; i < dataLength; i++) {
    byte a = EEPROM.read(address + 1 + i);
    data += char(a);
  }
  return data; //string
  }

