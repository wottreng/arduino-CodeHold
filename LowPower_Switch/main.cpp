#include <Arduino.h>
#include <LowPower.h>
#include <string.h>
/*
* hardware: arduino pro mini 8Mhz 3.3V: https://amzn.to/3IDkcBo
            usb to uart module: https://amzn.to/3G2czTF
            315Mhz transmitter: https://amzn.to/3KJkp7X
            light switch: https://amzn.to/3H5FVlo
            housing: https://amzn.to/3FVdwgt

* software: c++ compiled with PlatformIO
* lib dependandents:
  arduino_low_power : https://github.com/warhog/arduino-low-power
  RadioHead : https://www.airspayce.com/mikem/arduino/RadioHead/

uses 315mhz radio

** power usage ** (measured with multimeter)
4.5ma  arduino pro mini
2ma  arduino pro mini with low power enabled and power led
0.08 ma pro mini with low power sleep forever and power led removed
--
with 2 AA batterys in series for power: ~3.3v
assuming 1 amp-hour available from battery pack
1000mA/0.08 = 12500 hours or 520 days
with each use this will drop alot
--------

** arduino pro mini **
hardware ref: https://www.arduino.cc/en/pmwiki.php?n=Main/ArduinoBoardProMini
interupt pins: 2 or 3

** Connections during flash **
FT323 to arduino pro mini
CTS -> GND
RTS -> DTR
RX -> TX
TX -> RX
GND -> GND
VCC -> VCC
-------------
*/

// for serial output during developement, make debug true
#define debug false

#define switchPin 2 // rocker switch input pin

// Include RadioHead Amplitude Shift Keying Library
#include <RH_ASK.h>
// Include dependant SPI Library
#include <SPI.h>

// Create Amplitude Shift Keying Object
#define transmitter_pin 3 // connected to 315Mhz transmitter data line
#define transmission_rate 2000 // bits per second
RH_ASK driver(transmission_rate, 0, transmitter_pin, 0); // (bits/sec, rxPin, txPin(D3), pttPin)
/*
NOTE: if reducing clock speed, transmission rate needs to be inverse
      for example, if reducing clock from 8 to 4 Mhz, transmission speed needs to double
*/

// send RF message
void send_message()
{
    for (uint8_t i = 0; i < 1; i++) { // sends message twice, I found that the first message doesnt always make it so a second one helps with redundancy
        String message = String(i); // sends 0 then 1, set to what ever you want
        const char* msg = message.c_str(); // convert string to char array
        driver.send((uint8_t*)msg, strlen(msg)); // send message
        driver.waitPacketSent(); // wait until message is sent
#if debug
        Serial.println("sent");
#endif
        LowPower.idle(SLEEP_500MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); 
    }
}

void wakeUp()
{
    // Just a handler for the pin interrupt.
}

void setup()
{
    pinMode(switchPin, INPUT_PULLUP); // pulled high by internal resistor
    //
    driver.init(); // rf driver
    //
#if debug
    Serial.begin(9600); // does not work with slower clock speed
    Serial.println("start");
#endif
}

void loop()
{
#if debug
    Serial.println("loop start");
#endif

    attachInterrupt(digitalPinToInterrupt(switchPin), wakeUp, CHANGE);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(switchPin);
    send_message(); // send RF message
    LowPower.idle(SLEEP_2S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF); //debounce

#if debug
    Serial.println("end loop");
#endif
}
