#include <Wire.h>
#include <SI7021.h>


SI7021 sensor;
int led1 = 3;
int led2 = 4;

void setup() {
    pinMode(led1, OUTPUT);
    pinMode(led2, OUTPUT);
    sensor.begin();
}


void loop() {

    // temperature is an integer in hundredths
    int temperature = sensor.getCelsiusHundredths();
    temperature = temperature / 100;
    for (int i = 0; i < temperature; i++) {
        pulse(led1); 
    }
    
    delay(5000);
    
    // humidity is an integer representing percent
    int humidity = sensor.getHumidityPercent();
    for (int i = 0; i < humidity; i++) {
        pulse(led2); 
    }
    
    delay(5000);
    
    // this driver should work for SI7020 and SI7021, this returns 20 or 21
    int deviceid = sensor.getDeviceId();
    for (int i = 0; i < deviceid; i++) {
        pulse(led1); 
    }
    delay(5000);

    // enable internal heater for testing
    sensor.setHeater(true);
    delay(20000);
    sensor.setHeater(false);
    
    // see if heater changed temperature
    int temperature = sensor.getCelsiusHundredths();
    temperature = temperature / 100;
    for (int i = 0; i < temperature; i++) {
        pulse(led2); 
    }
    
    //cool down
    delay(20000);

    // get humidity and temperature in one shot, saves power because sensor takes temperature when doing humidity anyway
    si7021_env data = sensor.getHumidityAndTemperature();
    for (int i = 0; i < data.celsiusHundredths/100; i++) {
        pulse(led1); 
    }
    for (int i = 0; i < data.humidityPercent; i++) {
        pulse(led2); 
    }
    delay(5000);
}

void pulse(int pin) {
   // software pwm, flash light for ~1 second with short duty cycle
   for (int i = 0; i < 20; i++) {
       digitalWrite(pin, HIGH);
       delay(1);
       digitalWrite(pin,LOW);
       delay(9);
   }
   digitalWrite(pin,LOW);
   delay(300);
}


