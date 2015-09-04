# MockMySensors
Arduino sketch to fake sensor for the MySensors Library

This skecth is intended to crate fake sensors which register and respond to the controller

Barduino, GizMoCuz 2015
-----------------

Comment/Uncomment the sensors you would like to test

Arduino Uno R3 can support about 12 sensors before it runs out of memory.
You probably need to turn of the MySensor Lib debug to save some memory

Nest step will be to remove the read/write to the eprom
and extend 'ID_S_HEATER' to support the v1.5 API
