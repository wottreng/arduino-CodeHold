#include <Arduino.h>
#include <string.h>
#include <math.h>

#include "myWeather.h"

// local variables -------------------------
extern bool debug ;

// pins
const byte windspeedPin = 14; // interupt, D5
const byte rainBucketPin = 12; // interupt, D6
const byte windDirPin = A0;

// time
byte seconds = 0; // When it hits 60, increase the current minute
byte minutes = 0; // Keeps track of where we are in various arrays of data
byte seconds_2m = 0; // Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes_10m = 0; // Keeps track of where we are in wind gust/dir over last 10 minutes array of data
byte minutes_60m = 0; // keep track of every hour
byte hour_24h = 0; // keep track of hour in day

// var check intervals
unsigned long last_wind_check = 0;
volatile unsigned long last_wind_IRQ = 0;

// wind speed
volatile byte wind_clicks = 0; // wind IR tracker
byte wind_spd_avg_120s[120]; // 120 bytes to keep track of 2 minute average
float wind_gust_10m[10]; // 10 floats to keep track of 10 minute max
float wind_spd_mph_avg2m = 0; // [mph 2 minute average wind speed mph]
float wind_speed_mph_now = 0; // [mph instantaneous wind speed]
float wind_gust_mph_daily_max = 0; // [mph current wind gust for the day]
float wind_gust_mph_10m_max = 0; // [mph past 10 minutes wind gust mph ]

// wind direction
int wind_dir_avg_120s[120]; // 120 ints to keep track of 2 minute average
int wind_gust_direction_10m[10]; // 10 ints to keep track of 10 minute max
int wind_dir_now = 0; // [0-360 instantaneous wind direction]
int wind_dir_avg2m = 0; // [0-360 2 minute average wind direction]
int wind_gust_dir_daily = 0; // [0-360 using software specific time period]
int wind_gust_dir_10m_average = 0; // [0-360 past 10 minutes wind gust direction]

// rain
volatile unsigned long rain_time, rain_last, rain_interval, rain;
volatile float rain_60m_array[60]; // 60 floating numbers to keep track of 60 minutes of rain
volatile float rain_daily_inches = 0; // [rain inches so far today in local time]
float rain_hourly_inches = 0; // [rain inches over the past hour)] -- the accumulated rainfall in the past 60 min
float rain_24h[24]; // rain per hour for 24 hours

// temp and humidity
float humidity = 0; // [%]
float temp_F = 0; // [temperature F]
float temp_60m[60]; // temp every minute
float humidity_60m[60];
float temp_24h[24];
float humidity_24h[24];

// pressure **
// float baromin = 30.03;// [barom in] - It's hard to calculate baromin locally, do this in the agent
// float pressure = 0;
// float dewptf; // [dewpoint F] - It's hard to calculate dewpoint locally, do this in the agent

// last check timers
unsigned long last60secCheck = 0;
unsigned long last1secCheck = 0; // The millis counter to see when a second rolls by

//Interrupt routines (these are called by the hardware interrupts, not by the main code)
IRAM_ATTR void rainIRQ()
{
    // Count rain gauge bucket tips as they occur
    // Activated by the magnet and reed switch in the rain gauge, attached to input D2
    if (millis(); - rain_last > 10) { // debounce
        rain_daily_inches += 0.011; // Each dump is 0.011" of water
        rain_60m_array[minutes] += 0.011; // Increase this minute's amount of rain
        rain_last = millis(); // set up for next event
    }
}

IRAM_ATTR void wspeedIRQ()
{
    // Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
    if (millis() - last_wind_IRQ > 10) { // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
        wind_clicks++; // There is 1.492MPH for each click per second.
        last_wind_IRQ = millis(); // Grab the current time
    }
}

// Setup =====================================================

// setup pins
void myWeatherInit(bool debugInput = false)
{
  if(debugInput) debug = true;
  //
  pinMode(windspeedPin, INPUT_PULLUP);
  pinMode(rainBucketPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rainBucketPin), rainIRQ, FALLING);
  attachInterrupt(digitalPinToInterrupt(windspeedPin), wspeedIRQ, FALLING);
  //
  last1secCheck = millis();
  last60secCheck = millis();
}

// Loop ======================================================

// 1 second loop timer
void myWeatherLoop1s()
{
    if (millis() - last1secCheck >= 1000) { // must be every second
      last1secCheck += 1000;
      updateWeather_EverySecond();
    }
}

// 60 second loop timer
void myWeatherLoop60s()
{
    if ((millis() - last60secCheck) >= 60000) {
        last60secCheck += 60000;
        calcWeather();  
    }
}

// functions -----------------------------------------

String getWindDataHTML() {
  return "<p class='textBlocks'>Current Speed: " + String(wind_speed_mph_now, 2) +
         " mph</p>"
         "<p class='textBlocks'>Average 2 min Speed: " +
         String(wind_spd_mph_avg2m, 2) +
         " mph</p>"
         "<p class='textBlocks'>Current Direction: " +
         get_cardinal_dir(wind_dir_now) + ", " + String(wind_dir_now) +
         "°</p>"
         "<p class='textBlocks'>Average 2 min Direction: " +
         get_cardinal_dir(wind_dir_avg2m) + ", " + String(wind_dir_avg2m) +
         "°</p>"
         "<p class='textBlocks'>Daily Gust Speed: " +
         String(wind_gust_mph_daily_max, 2) +
         " mph</p>"
         "<p class='textBlocks'>Gust Direction: " +
         get_cardinal_dir(wind_gust_dir_daily) + ", " + String(wind_gust_dir_daily) +
         "°</p>"
         "<p class='textBlocks'>Max 10 min Gust Speed : " +
         String(wind_gust_mph_10m_max, 2) +
         " mph</p>"
         "<p class='textBlocks'>Average 10 min Gust Direction : " +
         get_cardinal_dir(wind_gust_dir_10m_average) + ", " + String(wind_gust_dir_10m_average) +
         "°</p>";
}

String getRainDataHTML() {
  return "<p class='textBlocks'>Hourly: " + String(rain_hourly_inches, 2) +
         " inches</p>"
         "<p class='textBlocks'>Daily: " +
         String(rain_daily_inches, 2) + " inches</p>";
}

String getWeatherDataJSON(){
  return  
   "\"wind_dir_now\":\"" + String(wind_dir_now) +"\","
   "\"wind_dir_avg2m\":\"" + String(wind_dir_avg2m) +"\","
   "\"wind_speed_mph_now\":\"" + String(wind_speed_mph_now, 2) +"\","
   "\"wind_spd_mph_avg2m\":\"" + String(wind_spd_mph_avg2m, 2) +"\","
   "\"wind_gust_mph_daily_max\":\"" + String(wind_gust_mph_daily_max, 2) +"\","
   "\"wind_gust_direction_daily\":\"" + String(wind_gust_dir_daily) +"\","
   "\"wind_gust_mph_10m_max\":\"" + String(wind_gust_mph_10m_max, 2) +"\","
   "\"wind_gust_dir_10m_average\":\"" + String(wind_gust_dir_10m_average) +"\","
   "\"rain_hourly_inches\":\"" + String(rain_hourly_inches, 2) +"\","
   "\"rain_daily_inches\":\"" + String(rain_daily_inches, 2) +"\","; 
}

//
void updateWeather_EverySecond(){
    
    //Take a speed and direction reading every second for 2 minute average
    if (++seconds_2m >= 120) seconds_2m = 0;
    
    wind_speed_mph_now = get_wind_speed();
    int wind_dir_now = get_wind_direction();

    //Calc the wind speed and direction every second for 120 second to get 2 minute average
    wind_spd_avg_120s[seconds_2m] = (int)wind_speed_mph_now;
    wind_dir_avg_120s[seconds_2m] = wind_dir_now;

    // Check to see if wind speed now is greater than max wind gust for the minute
    if (wind_speed_mph_now > wind_gust_10m[minutes_10m])
    {
      wind_gust_10m[minutes_10m] = wind_speed_mph_now;
      wind_gust_direction_10m[minutes_10m] = wind_dir_now;
    }

    // Check to see if wind speed now is greater than max wind gust for the day
    if (wind_speed_mph_now > wind_gust_mph_daily_max) {
      wind_gust_mph_daily_max = wind_speed_mph_now;
      wind_gust_dir_daily = wind_dir_now;
    }
    // if over 1 minute, calculate minute 
    if (++seconds > 59) {

      seconds = 0;

      if (++minutes > 59) minutes = 0;
      if (++minutes_10m > 9) minutes_10m = 0;
      if (++minutes_60m > 59){
        minutes_60m = 0;
        rain_24h[hour_24h] = rain_hourly_inches;
        rain_hourly_inches = 0;
        ++hour_24h;
      }
      rain_60m_array[minutes] = 0; //Zero out this minute's rainfall amount
      wind_gust_10m[minutes_10m] = 0; //Zero out this minute's gust
    }  
}

// 
float get_wind_speed(){
  float deltaTime = millis() - last_wind_check; //750ms
  deltaTime /= 1000.0; //Covert to seconds
  float windSpeed = (float)wind_clicks / deltaTime; //3 / 0.750s = 4
  wind_clicks = 0; //Reset and start watching for new wind
  last_wind_check = millis();
  windSpeed *= 1.492; //4 * 1.492 = 5.968MPH
  return (windSpeed);
}

// 
int get_wind_direction(){
  unsigned int adc;
  adc = analogRead(windDirPin); // get the current reading from the sensor

  // NEED TO DO MEASUREMENTS
  // ADD 1K resitor inline, try 3.3 volts input
  // N: 33, NE:117, E:545, SE:341, S: 223, SW:65, W:10, NW: 19  
  if (adc < 12) return (270); //W  10
  if (adc < 23) return (315); //NW  19
  if (adc < 35) return (0); //N  33
  if (adc < 70) return (225); //SW  65
  if (adc < 125) return (45); //NE  117
  if (adc < 245) return (180); //S 223
  if (adc < 360) return (135); //SE 341 
  if (adc < 650) return (90); // E 545
  
  return (-1); // error, disconnected?
}

// degrees to cardinal directions
String get_cardinal_dir(int dir){
  if (dir < 22) return ("North"); 
  if (dir < 67) return ("NorthEast"); 
  if (dir < 112) return ("East"); 
  if (dir < 157) return ("SouthEast"); 
  if (dir < 202) return ("South");
  if (dir < 247) return ("SouthWest"); 
  if (dir < 292) return ("West"); 
  if (dir < 337) return ("NorthWest"); 
  if (dir <= 360) return ("North"); 
  return ("Error");
}

// called every 60 sec
void calcWeather(){
  //
  wind_dir_now = get_wind_direction();

  //Calc wind speed mph average for 2 min
  float wind_Speed_total = 0;
  for (int i = 0; i < 120; i++) {
      wind_Speed_total += wind_spd_avg_120s[i]; // total = total + average[i]
  }
  wind_spd_mph_avg2m = wind_Speed_total / 120.0;

  //Calc wind_dir_avg_2m, Wind Direction
  //You can't just take the average. Google "mean of circular quantities" for more info
  //We will use the Mitsuta method because it doesn't require trig functions
  //And because it sounds cool.
  //Based on: http://abelian.org/vlf/bearings.html
  //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
  long sum = wind_dir_avg_120s[0];
  int Dir = wind_dir_avg_120s[0];

  for (byte i = 1 ; i < 120 ; i++)  {
    int delta = wind_dir_avg_120s[i] - Dir;
    if (delta < -180) Dir += delta + 360;
    else if (delta > 180) Dir += delta - 360;
    else Dir += delta;
    sum += Dir;
  }
  // calc average wind direction for the last two min
  wind_dir_avg2m = sum / 120;
  // correct for greater than 360 or less than 0
  if (wind_dir_avg2m >= 360) wind_dir_avg2m -= 360;
  if (wind_dir_avg2m < 0) wind_dir_avg2m += 360;

  //Find the largest windgust in the last 10 minutes
  wind_gust_mph_10m_max = 0;
  wind_gust_dir_10m_average = 0;

  for (byte i = 0; i < 10 ; i++) {
    if (wind_gust_10m[i] > wind_gust_mph_10m_max) {
      wind_gust_mph_10m_max = wind_gust_10m[i];
      wind_gust_dir_10m_average = wind_gust_direction_10m[i];
    }
  }

  //Calculate amount of rainfall for the last 60 minutes
  rain_hourly_inches = 0;
  for (int i = 0 ; i < 60 ; i++){
    rain_hourly_inches += rain_60m_array[i];
  }

}


