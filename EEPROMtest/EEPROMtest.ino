#include <EEPROM.h>

//eeprom variables
byte ssidLength;
byte passLength;
String WIFI_SSID;
String WIFI_PASS;

void setup() {
  //int val = 1;
  Serial.begin(115200);
  delay(100);
  //Serial.println("--------------");
  EEPROM.begin(512);
  //------------------------
  WIFI_SSID = "good3bye234";
  WIFI_PASS = "th6543ere9";

  //------------------write stuff---------------------------
  ssidLength = WIFI_SSID.length();
  char ssidInput[ssidLength + 1];
  passLength = WIFI_PASS.length();
  char passInput[passLength + 1];

  // string to char array
  strcpy(ssidInput, WIFI_SSID.c_str());
  strcpy(passInput, WIFI_PASS.c_str());

  Serial.println("--------");
  Serial.println("--write stuff --------");

  EEPROM.write(0x32, ssidLength);
  for (int i = 0; i < ssidLength; i++) {
    EEPROM.write(0x33 + i, ssidInput[i]);
  }
  EEPROM.write(0x46, passLength);
  for (int i = 0; i < passLength; i++) {
    EEPROM.write(0x47 + i, passInput[i]);
  }
  EEPROM.commit();//flush write content to eeprom
  delay(1000);//+++++++++++++++++++++++++++++++++
  //--------------------------------------------------------
  //-------read stuff----------------------------------------
  Serial.println("--------");
  Serial.println("--read stuff --------"); 
  //------------------------------------
  ssidLength = EEPROM.read(0x32);
  WIFI_SSID = "";
  for (int i = 0; i < ssidLength; i++) {
    byte a = EEPROM.read(0x33 + i);
    WIFI_SSID += char(a);
  }
  Serial.println("-----ssid output-----");
  Serial.println(WIFI_SSID);

  passLength = EEPROM.read(0x46);
  WIFI_PASS = "";
  for (int i = 0; i < passLength; i++) {
    byte a = EEPROM.read(0x47 + i);
    WIFI_PASS += char(a);
  }
  Serial.println("-----pass output-----");
  Serial.println(WIFI_PASS);

}

void loop() {
  delay(10000);
  Serial.println("done");
}
