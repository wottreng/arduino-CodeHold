/*

drive relay and onboard LED: 116ma
drive relay, no LED: 113ma
idle: 28.6ma

nano interrupt pins: 2,3

*/
bool debug = false;


const int relayPin = 2;
const int motionDetectorPin = 3;
const int photoresistorPin = A0;
int light = 0; //dark=>34, medium=>[155,188], bright => 725
int tooDark = 160;
//byte motionDetected = 0;
unsigned long checkLightSensorTime = 0;
unsigned long motionDetectedTime = 0;
unsigned long oldtime = 0;
unsigned long timeOn = 480000; // 8min
unsigned long debugTime = 0;

void setup() {  
  pinMode(motionDetectorPin, INPUT);
  //pinMode(photoresistorPin, INPUT);
  //attachInterrupt(digitalPinToInterrupt(motionDetectorPin), motionDetectedISR, RISING);
  //attachInterrupt(motionDetectorPin, motionDetectedISR, RISING);
  pinMode(relayPin, OUTPUT);  
  digitalWrite(relayPin, 0);
  if (debug){
    timeOn = 10000;    
    Serial.begin(9600);  
    Serial.println("starting");
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, 0);     
  }  
}

void loop() {
  if(debug && (millis() - debugTime) > 5000){
    Serial.println("-------------------");
    Serial.println("relay state:" + String(digitalRead(relayPin)));
    Serial.println("motion: " + String(digitalRead(motionDetectorPin)));
    Serial.println("time since last motion: " + String((millis() - motionDetectedTime)/1000) + " sec");
    Serial.println("light sensor output: " + String(light));
    Serial.println("-------------------");
    debugTime = millis();
  }
  if((millis() - checkLightSensorTime) > 3000){
    light = analogRead(photoresistorPin);
    checkLightSensorTime = millis();
  }
  
  if(digitalRead(motionDetectorPin) == 1 ){ //motion detected
    
    if(digitalRead(relayPin) == 1){ // relay already on
      motionDetectedTime = millis(); // reset on time delay
      if(debug){
        Serial.println("reset on time delay");
      }
      delay(1000);
    }    
    else if(digitalRead(relayPin) == 0 && light < tooDark){ // relay is off
      activateRelay(1);
      motionDetectedTime = millis();
      if(debug){
        Serial.println("turn relay on");
      }              
    }
    else{
      if(debug){
        Serial.println("motion detected but too bright");
      }
      delay(100);
    }
  }
  // turn relay off after timeon delay and no motion
  if((millis() - motionDetectedTime) > timeOn && digitalRead(relayPin) == 1 && digitalRead(motionDetectorPin) == 0){
    activateRelay(0);
    if(debug){
      Serial.println("turn relay off");
    }
  }  
}

void activateRelay(byte State){
  if(debug){
    digitalWrite(LED_BUILTIN, State); 
  }  
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

