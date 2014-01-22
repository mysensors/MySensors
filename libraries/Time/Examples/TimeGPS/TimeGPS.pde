/*
 * TimeGPS.pde
 * example code illustrating time synced from a GPS
 * 
 */

#include <Time.h>
#include <TinyGPS.h>       //http://arduiniana.org/libraries/TinyGPS/
#include <NewSoftSerial.h>  //http://arduiniana.org/libraries/newsoftserial/
// GPS and NewSoftSerial libraries are the work of Mikal Hart

TinyGPS gps; 
NewSoftSerial serial_gps =  NewSoftSerial(3, 2);  // receive on pin 3

const int offset = 1;   // offset hours from gps time (UTC)
time_t prevDisplay = 0; // when the digital clock was displayed

void setup()
{
  Serial.begin(9600);
  serial_gps.begin(4800);
  Serial.println("Waiting for GPS time ... ");
  setSyncProvider(gpsTimeSync);
}

void loop()
{
  while (serial_gps.available()) 
  {
    gps.encode(serial_gps.read()); // process gps messages
  }
  if(timeStatus()!= timeNotSet) 
  {
     if( now() != prevDisplay) //update the display only if the time has changed
     {
       prevDisplay = now();
       digitalClockDisplay();  
     }
  }	 
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

time_t gpsTimeSync(){
  //  returns time if avail from gps, else returns 0
  unsigned long fix_age = 0 ;
  gps.get_datetime(NULL,NULL, &fix_age);
  unsigned long time_since_last_fix;
  if(fix_age < 1000)
    return gpsTimeToArduinoTime(); // return time only if updated recently by gps  
  return 0;
}

time_t gpsTimeToArduinoTime(){
  // returns time_t from gps date and time with the given offset hours
  tmElements_t tm;
  int year;
  gps.crack_datetime(&year, &tm.Month, &tm.Day, &tm.Hour, &tm.Minute, &tm.Second, NULL, NULL);
  tm.Year = year - 1970; 
  time_t time = makeTime(tm);
  return time + (offset * SECS_PER_HOUR);
}
