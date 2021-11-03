/*
  //Connections
  Power (5V) is provided to the Arduino through usb

  Gyro - Arduino uno
  VCC  -  5V
  GND  -  GND
  SDA  -  A4 data
  SCL  -  A5 clock

  attach motor relays to digital pins
  FLmotor = 2, FRmotor = 3, RLmotor = 4, RRmotor = 5;

*//////////////////////////////////////////////////////////////////////////////////////

//Library
#include <Wire.h>

byte gyroAdd = 0x68;
//Declaring some global variables
//int gyro_x, gyro_y, gyro_z;
long acc_x, acc_y, acc_z; //acc_total_vector;
int temperature;
//long gyro_x_cal, gyro_y_cal, gyro_z_cal;
long loop_timer;
//int lcd_loop_counter;
//float angle_pitch, angle_roll;
//int angle_pitch_buffer, angle_roll_buffer;
//boolean set_gyro_angles;
//float angle_roll_acc, angle_pitch_acc;
//float angle_pitch_output, angle_roll_output;
float angle_x, angle_y, angle_tot, temp, diff = 0.2, oldAngle;//difference in angle
byte fid = 50;//sensor samples at 1000hz
byte FLmotor = 2, FRmotor = 3, RLmotor = 4, RRmotor = 5;
byte level = 0;//logic for setup


//=====================================================
void setup() {
  Wire.begin();     //Start I2C as master
  Serial.begin(57600);   //Use only for debugging
  Serial.println("begin level program");
  pinMode(FLmotor, OUTPUT);
  pinMode(FRmotor, OUTPUT);
  pinMode(RLmotor, OUTPUT);
  pinMode(RRmotor, OUTPUT);
  digitalWrite(FLmotor, LOW);
  digitalWrite(FRmotor, LOW);
  digitalWrite(RLmotor, LOW);
  digitalWrite(RRmotor, LOW);
  setup_mpu_6050_registers();     //Setup the registers of the MPU-6050 (500dfs / +/-8g) and start the gyro

  Serial.println("MPU-6050 IMU V1 initial angle calc");
  Serial.println("setup feet");
  feetSetup();//feet to ground
  //gyroCal();

  //pinMode(LED_BUILTIN, LOW);

  //loop_timer = millis();       //Reset the loop timer
}//==================================

void loop() {

  while (level == 0) {
    calcAngle();
    write_Serial(); //Write the roll and pitch values to the display
    levelFeet();
  }
  level = 1;
  delay(10000);//do nothing when done

}

//==subroutines=========================================================================
void read_mpu_6050_data() {                                            //Subroutine for reading the raw gyro and accelerometer data
  Wire.beginTransmission(gyroAdd);                                        //Start communicating with the MPU-6050
  Wire.write(0x3B);                                                    //Send the requested starting register
  Wire.endTransmission();                                              //End the transmission
  Wire.requestFrom(gyroAdd, 14);                                          //Request 14 bytes from the MPU-6050
  while (Wire.available() < 14);                                       //Wait until all the bytes are received
  acc_x = Wire.read() << 8 | Wire.read();                              //Add the low and high byte to the acc_x variable
  acc_y = Wire.read() << 8 | Wire.read();                              //Add the low and high byte to the acc_y variable
  acc_z = Wire.read() << 8 | Wire.read();                              //Add the low and high byte to the acc_z variable
  temperature = Wire.read() << 8 | Wire.read();                        //Add the low and high byte to the temperature variable
  // gyro_x = Wire.read() << 8 | Wire.read();                             //Add the low and high byte to the gyro_x variable
  //gyro_y = Wire.read() << 8 | Wire.read();                             //Add the low and high byte to the gyro_y variable
  //gyro_z = Wire.read() << 8 | Wire.read();                             //Add the low and high byte to the gyro_z variable

}
//===========
void calcAngle() {
  //Subtract the offset calibration value
  for (byte x = 0; x < fid ; x ++) {
    read_mpu_6050_data(); //read sensor
    //gyro_x -= gyro_x_cal;
    //gyro_y -= gyro_y_cal;
    //gyro_z -= gyro_z_cal;
    angle_x += acc_x / 182.04;//calc angle in degrees
    angle_y += acc_y / 182.04;
    //angle_z += acc_z / 182.04;
    temp += temperature;
    delay(10);
  }
  angle_x /= fid;
  angle_y /= fid;
  angle_tot = angle_x + angle_y;
  //angle_z /= fid;
}
//=======================================
void  levelFeet() {
  //angle_x,y,z
  //FLmotor,FRmotor,RLmotor,RRmotor
  //logic x is roll, y is pitch
  calcAngle();//initial calc
  while (level == 0) {
    calcAngle();

    if (angle_x < 0.0 && angle_y < 0.0) {
     digitalWrite(FRmotor, HIGH);
    }
    if (angle_x < 0.0 && angle_y > 0.0) {
      digitalWrite(RRmotor, HIGH);
    }
    if (angle_x > 0.0 && angle_y < 0.0) {
      digitalWrite(FLmotor, HIGH);
    }
    if (angle_x > 0.0 && angle_y > 0.0) {
     digitalWrite(RLmotor, HIGH);
    }
    oldAngle = angle_tot;
    while (angle_tot < oldAngle + diff && angle_tot > oldAngle - diff) {
      calcAngle();
      Serial.print(".");
    }
    Serial.println("loop");
    write_Serial();
    //logic to turn off motors
    if (digitalRead(FLmotor) == HIGH){ digitalWrite(FLmotor, LOW);}
    if (digitalRead(FRmotor) == HIGH){ digitalWrite(FRmotor, LOW);}
    if (digitalRead(RLmotor) == HIGH){ digitalWrite(RLmotor, LOW);}
    if (digitalRead(RRmotor) == HIGH){ digitalWrite(RRmotor, LOW);}
    if (angle_x > -0.5 && angle_x < 0.5) {
      if (angle_y > -0.5 && angle_y < 0.5) {
        level = 1; //finished leveling
      }
    }
  }

  Serial.println("finished");
}
//==========================================
void feetSetup() { //initial feet setup
  //angle_x,y,z
  //FLmotor,FRmotor,RLmotor,RRmotor
  calcAngle();//initial calc
  for (byte x = 0; x < 4 ; x ++) {
    calcAngle();

    if (x == 0) {
      Serial.println("FL high");
      digitalWrite(FLmotor, HIGH);
    }
    if (x == 1) {
      Serial.println("FR high");
      digitalWrite(FLmotor, LOW);
      digitalWrite(FRmotor, HIGH);
    }
    if (x == 2) {
      Serial.println("RR high");
      digitalWrite(FRmotor, LOW);
      digitalWrite(RRmotor, HIGH);
    }
    if (x == 3) {
      Serial.println("RL high");
      digitalWrite(RRmotor, LOW);
      digitalWrite(RLmotor, HIGH);
    }
    oldAngle = angle_tot;
    while (angle_tot < oldAngle + diff && angle_tot > oldAngle - diff) {
      calcAngle();
      Serial.println("waiting");
    }
    Serial.println("");
  }
  digitalWrite(RLmotor, LOW);
  Serial.println("finished");
}
//=======================================
void write_Serial() {
  //
  Serial.print("angle x: ");
  Serial.println(angle_x);
  Serial.print("angle y: ");
  Serial.println(angle_y);
  //Serial.print("temp: ");
  //Serial.println(temperature);
  Serial.println("");
}
//==========================================
void setup_mpu_6050_registers() {
  //Activate the MPU-6050
  Wire.beginTransmission(gyroAdd);     //Start communicating with the MPU-6050
  Wire.write(0x6B); //reg 107, pwr mgmnt
  Wire.write(0x00); //run mpu at 8mhz, internal clock
  Wire.endTransmission();
  //Configure the accelerometer (+/-2g)
  Wire.beginTransmission(gyroAdd);
  Wire.write(0x1C); //reg 28
  Wire.write(0x00); //00000000    //Set the requested starting register
  Wire.endTransmission();
  //Configure the gyro (250dps full scale)
  Wire.beginTransmission(gyroAdd);
  Wire.write(0x1B); //reg 27
  Wire.write(0x00); // 00000000
  Wire.endTransmission();
}
//===========================
