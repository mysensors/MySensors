# MySensors example

This example shows how HeatpumpIR could be integrated with the MySensors sensor network. As this sensor sends infrared signals, 
the natural sensor type is 'S_IR'. The heatpump/air conditioner signals are really long, so the signals need to be encoded somehow.

In this example the idea is to encode just this information, using 24 bits of data
* model code (see the index of the array heatpumpIR in the code)
* power state (for the rest, see the HeatpumpIR.h file for the constants)
* operating mode
* fan speed
* temperature (the temperature is directly the temperature in HEX)

```
An example:
00213416 (as an example of a valid code)
  2 = PanasonicJKE
   1 = Power ON
    3 = COOL
     4 = FAN 4
      16 = Temperature 22 degrees (0x16 = 22)
```

This sketch creates three sensors:
* S_IR
* S_INFO
* S_LIGHT

The S_INFO and S_LIGHT sensors are for Domoticz, as it does not currently (as of V2.3093) support infrared code sending through the S_IR sensor type.

## Sending the code
### MYSController

* Select the IR sensor on the 'Heatpump Sensor 1.0' node
* Select 'IR_SEND' as the subtype
* Enter for example '00213416' as the payload

### Domoticz

As Domoticz does not really support sending IR code yet, the workaround is to use two sensors. When the node is connected, Domoticz should find two new devices
* A 'Lighting 2 / AC' type sensor with name 'IR send'
* A 'General / Text' type sensor with name 'Unknown' and Data '00000000'

To send a command, first add the sensors. Then update the data of the text sensor to contain the IR code to be sent, and then flip the switch to trigger the sending.
The data can be updated through the JSON REST API, or through the Lua event scripts.

REST API example (assuming that the idx of the text sensor is 105 and the switch is 104, and Domoticz is at 192.168.0.4:8080):
```
http://192.168.0.4:8080/json.htm?type=command&param=udevice&idx=105&nvalue=0&svalue=00213416
http://192.168.0.4:8080/json.htm?type=command&param=switchlight&idx=104&switchcmd=On
```

Lua example:
* Seems that the text sensor can't be updated through the commandArray...
* So now when a device called 'trigger' changes, the text sensor value is first updated to '00213416', and then the IR signal is sent

```
commandArray = {}

for key, value in pairs(devicechanged) do
  if (key == 'trigger') then

    print("Heatpump script")
    commandArray['OpenURL']='http://192.168.0.4:8080/json.htm?type=command&param=udevice&idx=105&nvalue=0&svalue=00213416'
    commandArray['IR send']='On'
  end
end

return commandArray
```

## Hardware

Use any ATMega328-based Arduino, connect the radio, and finally connect an infrared led (in series with a resistor, like 100 Ohm) between digital pin 3 and GND.