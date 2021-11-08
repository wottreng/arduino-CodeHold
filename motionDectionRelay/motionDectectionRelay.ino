/*
Motion and light sensor relay program

hardware:
arduino nano
Onyehn IR Pyroelectric Infrared PIR Motion Sensor Detector Module
photoresistor 
5v one channel relay module board

power consumption for arduino nano:
drive relay and onboard LED: 116ma
drive relay, no LED: 113ma
idle: 28.6ma
*/
bool debug = false;

byte relayOn = 0;
const byte relayPin = 2;
const byte motionDetectorPin = 3;
const byte photoresistorPin = 0;
byte light = 0; //dark=>34, medium=>[123,188], bright => 725
byte motionDetected = 0;
volatile unsigned long motionDetectedTime = 0;
unsigned long oldtime = 0;
unsigned long timeOn = 300000; // 5min on time delay 
unsigned long debugTime = 0;

void setup() {  
  pinMode(motionDetectorPin, INPUT);
  pinMode(photoresistorPin, INPUT);
  //attachInterrupt(digitalPinToInterrupt(motionDetectorPin), motionDetectedISR, RISING);
  //attachInterrupt(motionDetectorPin, motionDetectedISR, RISING);
  pinMode(relayPin, OUTPUT);  
  digitalWrite(relayPin, 0);
  if (debug){
    Serial.begin(9600);  
    Serial.println("starting");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, 0);     
  }  
}

void loop() {
  if(debug && (millis() - debugTime) > 5000){
    Serial.println("-------------------");
    Serial.println(relayOn);
    Serial.println(digitalRead(motionDetectorPin));
    Serial.println((millis() - motionDetectedTime));
    Serial.println(analogRead(photoresistorPin));
    Serial.println("-------------------");
    debugTime = millis();
  }
  
  if(digitalRead(motionDetectorPin) == 1 ){ //motion detected
    
    if(relayOn == 1){ // relay already on
      motionDetectedTime = millis(); // reset on time delay
      if(debug){
        Serial.println("reset on time delay");
      }
    }    
    if(relayOn == 0){ // relay is off
      light = analogRead(photoresistorPin);
      if (light < 100){
        activateRelay(1);
        relayOn = 1;
        motionDetectedTime = millis();
        if(debug){
          Serial.println("on");
        }        
      } 
      if(debug){
        Serial.println("relay off, light: "+ light);
      }     
    }
  }
  // turn relay off after timeon delay and no motion
  if(relayOn == 1 && digitalRead(motionDetectorPin) == 0 && (millis() - motionDetectedTime) > timeOn){
    activateRelay(0);
    relayOn = 0;
    if(debug){
      Serial.println("off");
    }
  }  
}

void activateRelay(byte State){
  digitalWrite(LED_BUILTIN, State); 
  digitalWrite(relayPin, State);
}

/*
void motionDetectedISR(){
  motionDetectedTime = millis();
  if(debug){
    Serial.println("ISR trigger: " + motionDetectedTime);
  }
}
*/

