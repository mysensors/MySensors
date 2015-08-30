# MockMySensors
Arduino sketch to fake sensor for the MySensors Library

This skecth is intended to crate fake sensors which register and respond to the controller

Barduino, GizMoCuz 2015
-----------------

Comment/Uncomment the sensors you would like to test

Arduino Uno R3 can support about 12 sensors before it runs out of memory.
(turning off the MySensors debug helps if you are memory limited)

With a Mega you can have them all


Changes Log
-----------------

2015-08-19
Removed the load/save to eeprom
Touched the heater sensor
Created a pseudo sensor to handle the V_ARMED message.
All sensors which support V_ARMED will arm/disarm at the same time
