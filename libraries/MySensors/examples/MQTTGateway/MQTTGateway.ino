/* 				MyMQTT Broker Gateway 0.1b

Created by Daniel Wiegert <daniel.wiegert@gmail.com>
Based on MySensors Ethernet Gateway by Henrik Ekblad <henrik.ekblad@gmail.com>
http://www.mysensors.org

Requires MySensors lib 1.4b

* Change below; TCP_IP, TCP_PORT, TCP_MAC
This will listen on your selected TCP_IP:TCP_PORT below, Please change TCP_MAC your liking also.
*1 -> NOTE: Keep first byte at x2, x6, xA or xE (replace x with any hex value) for using Local Ranges.

*2 You can use standard pin set-up as MySensors recommends or if you own a IBOARD you may change
	the radio-pins below if you hardware mod your iBoard. see [URL BELOW] for more details.
	http://forum.mysensors.org/topic/224/iboard-cheap-single-board-ethernet-arduino-with-radio/5

* Don't forget to look at the definitions in libraries\MySensors\MyMQTT.h!

	define TCPDUMP and connect serial interface if you have problems, please write on
	http://forum.mysensors.org/ and explain your problem, include serial output. Don't forget to
	turn on DEBUG in libraries\MySensors\MyConfig.h also.

	MQTT_FIRST_SENSORID is for 'DHCP' server in MyMQTT. You may limit the ID's with FIRST and LAST definition.
	If you want your manually configured below 20 set MQTT_FIRST_SENSORID to 20.
	To disable: set MQTT_FIRST_SENSORID to 255.

	MQTT_BROKER_PREFIX is the leading prefix for your nodes. This can be only one char if like.

	MQTT_SEND_SUBSCRIPTION is if you want the MyMQTT to send a empty payload message to your nodes.
	This can be useful if you want to send latest state back to the MQTT client. Just check if incoming
	message has any length or not.
	Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.

* Address-layout is : [MQTT_BROKER_PREFIX]/[NodeID]/[SensorID]/[SensorType]
	NodeID and SensorID is uint8 (0-255) number.
	Last segment is translation of the sensor type, look inside MyMQTT.cpp for the definitions.
	User can change this to their needs. We have also left some space for custom types.

Special: (sensor 255 reserved for special commands)
You can receive a node sketch name with MyMQTT/20/255/Sketch_name (or version with _version)

To-do:
Special commands : clear or set EEPROM Values, Send REBOOT and Receive reboot for MyMQTT itself.
Be able to send ACK so client returns the data being sent.
... Please come with ideas!

Test in more MQTT clients, So far tested in openhab and MyMQTT for Android (Not my creation)
- http://www.openhab.org/
- https://play.google.com/store/apps/details?id=at.tripwire.mqtt.client&hl=en
... Please notify me if you use this broker with other software.


* Example Openhab configuration:
openhab.cfg
---
mqtt:mysensor.url=tcp://192.168.0.234:1883
mqtt:mysensor.clientId=MQTT
---

items/test.items
---
Group test
Number Temp_test	"Temp [%.1f °C]"		(test)	{mqtt="<[mysensor:MyMQTT/20/#/Temperature:state:default]"}
Number Hum_test		"Hum [%.1f %%]"			(test)	{mqtt="<[mysensor:MyMQTT/20/#/Humidity:state:default]"}
Number test_test	"test [%s]"				(test)	{mqtt="<[mysensor:MyMQTT/20/3/#:state:default]"}
Switch sw1			"sw 1"     				(test)	{mqtt="<[mysensor:MyMQTT/21/1/Light:command:MAP(1on0off.map)]"}
Switch sw2	"sw2 recieve example"			(test) 	{mqtt=">[mysensor:MyMQTT/21/2/Light:command:ON:1],
	>[mysensor:MyMQTT/21/2/Light:command:OFF:0]"}
Switch sw3	"sw2 send + recieve example"	(test) 	{mqtt=">[mysensor:MyMQTT/21/2/Light:command:ON:1],
	>[mysensor:MyMQTT/21/2/Light:command:OFF:0],<[mysensor:MyMQTT/21/2/Light:command:MAP(1on0off.map)]"}
String sketch20		"Node 20 Sketch name [%s]"	(test)	{mqtt="<[mysensor:MyMQTT/20/255/Sketch_name:state:default]"}
String sketch21 	"Node 21 Sketch name [%s]"	(test)	{mqtt="<[mysensor:MyMQTT/21/255/Sketch_name:state:default]"}
--- (Note; # = Wildcard character)

sitemap/test.site
---
sitemap demo label="Menu"
Frame label="Openhab" {
		Group item=test label="Test group"
}
---

transform/1on0off.map
---
1=ON
0=OFF
---

*/


#include <SPI.h>
#include <MyMQTT.h>
#include <Ethernet.h>


// Use this for IBOARD modded to use standard MISO/MOSI/SCK, see note *1 above!
/*
#define RADIO_CE_PIN        3			// radio chip enable
#define RADIO_SPI_SS_PIN    8			// radio SPI serial select
#define RADIO_ERROR_LED_PIN A2  		// Error led pin
#define RADIO_RX_LED_PIN    A1  		// Receive led pin
#define RADIO_TX_LED_PIN    A0  		// the PCB, on board LED*/

//Use this for default configured pro mini / nano etc :
///*
#define RADIO_CE_PIN        5		// radio chip enable
#define RADIO_SPI_SS_PIN    6		// radio SPI serial select
#define RADIO_ERROR_LED_PIN 7		// Error led pin
#define RADIO_RX_LED_PIN    8		// Receive led pin
#define RADIO_TX_LED_PIN    9		// the PCB, on board LED*/

#define TCP_PORT 1883										// Set your MQTT Broker Listening port.
IPAddress TCP_IP ( 192, 168, 0, 234 );						// Configure your static ip-address here
byte TCP_MAC[] = { 0x02, 0xDE, 0xAD, 0x00, 0x00, 0x42 };	// Mac-address - You should change this! see note *2 above!

EthernetServer server = EthernetServer(TCP_PORT);
MyMQTT gw(RADIO_CE_PIN, RADIO_SPI_SS_PIN);

// -- Uncomment this constructor if you have leds and include button attached to your gateway
//MyMQTT gw(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN);

void processEthernetMessages() {
	char inputString[MQTT_MAX_PACKET_SIZE] = "";
	int inputSize = 0;
	EthernetClient client = server.available();
	if (client) {
		while (client.available()) {
			char inChar = client.read();
			inputString[inputSize] = inChar;
			inputSize++;
		}
		gw.processMQTTMessage(inputString, inputSize);
	}
}

void writeEthernet(const char *writeBuffer, int *writeSize) {
	server.write((const uint8_t *)writeBuffer, *writeSize); // Todo: Should this really be *writeSize?
}

int main(void) {
	init();
	Ethernet.begin(TCP_MAC, TCP_IP);
	delay(1000);   // Wait for Ethernet to get configured.
	gw.begin(RF24_PA_LEVEL_GW, RF24_CHANNEL, RF24_DATARATE, writeEthernet);
	server.begin();
	while (1) {
		processEthernetMessages();
		gw.processRadioMessage();
	}
}

