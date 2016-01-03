# Arduino Oregon Library:
##Tested Hardware:
- Arduino UNO
- 433Mhz receiver (connected on digital pin 2)
- Oregon sensors THGR228N

## Example without MySensors:
Serial output:
```
Brute Hexadecimal data from sensor:
1A2D10E2711860043CDA
Oregon ID: 226 Hexadecimal: E2
Sensor ID: 226 has been find in position EEPROM: 0
Oregon model: THGR228N
Oregon channel: 1
Oregon temperature: 18.70
Oregon humidity: 46
Oregon battery level: 90

----

Brute Hexadecimal data from sensor:
1A2D10B4201340052F43
Oregon ID: 180 Hexadecimal: B4
Sensor ID: 180 has been find in position EEPROM: 1
Oregon model: THGR228N
Oregon channel: 1
Oregon temperature: 13.20
Oregon humidity: 54
Oregon battery level: 90
```

## Example with MySensors library (www.mysensors.org):
Serial output:
```
Brute Hexadecimal data from sensor:
1A2D10E2711860043CDA
Oregon ID: 226 Hexadecimal: E2
Sensor ID: 226 has been find in position EEPROM: 0
Oregon model: THGR228N
Oregon channel: 1
Oregon temperature: 18.70
send: 10-10-0-0 s=0,c=1,t=0,pt=7,l=5,sg=0,st=ok:18.7
Oregon humidity: 46
send: 10-10-0-0 s=0,c=1,t=1,pt=7,l=5,sg=0,st=ok:46.0
Oregon battery level: 90
send: 10-10-0-0 s=0,c=1,t=24,pt=7,l=5,sg=0,st=ok:90.0

----

Brute Hexadecimal data from sensor:
1A2D10B4201340052F43
Oregon ID: 180 Hexadecimal: B4
Sensor ID: 180 has been find in position EEPROM: 1
Oregon model: THGR228N
Oregon channel: 1
Oregon temperature: 13.20
send: 10-10-0-0 s=1,c=1,t=0,pt=7,l=5,sg=0,st=ok:13.2
Oregon humidity: 54
send: 10-10-0-0 s=1,c=1,t=1,pt=7,l=5,sg=0,st=ok:54.0
Oregon battery level: 90
send: 10-10-0-0 s=1,c=1,t=24,pt=7,l=5,sg=0,st=ok:90.0
```
## Result on OpenHAB controller (With MySensors)
![Logo](http://i.imgur.com/Tsne6yv.png)
