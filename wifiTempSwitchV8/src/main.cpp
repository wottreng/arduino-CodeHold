/*

temp sensor, relay module

	temperature wifi switch
	
   pinMode(, OUTPUT/INPUT);pin# 1,3,15,13,12,14,2,0,4,5,16,9,10
   ADC0: analogRead(A0)
   interupt pins: D0-D8 = pin 16,5,4,

pins used: D5(14)[rx],D6(12)[relay],D0(LED,16), D2(4)[ds18b20], D4(2)[DHT], D1(5)[not used], D7(13)[not used]

*/

// custom libs
#include <myEEPROM.h>
#include <myWifi.h>
#include <myRelay.h>
#include <myTemp.h>
#include <myScheduler.h>


//=========================================================
void setup() {
  //
  EEPROMinit();

  //
  myRelay_init();

  //
  myWifi_Init(false);

  //
  myTemp_init();

  //
  myWifi_connectWifi();
}
//==================================================
void loop()
{
  //
  myWifi_Loop();
  // scheduler ----
  //mySchedule_1s();
  mySchedule_20s();
  mySchedule_60s();
  mySchedule_5min();
  //mySchedule_10min();
  // -----------
}
 
