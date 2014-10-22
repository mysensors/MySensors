/* 				MyMQTT Client Gateway 0.1b

 Created by Norbert Truchsess <norbert.truchsess@t-online.de>

 Based on MyMQTT-broker gateway created by Daniel Wiegert <daniel.wiegert@gmail.com>
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

 * Address-layout is : [MQTT_BROKER_PREFIX]/[NodeID]/[SensorID]/V_[SensorType]
 NodeID and SensorID is uint8 (0-255) number.
 Last segment is translation of the sensor type, look inside MyMQTT.cpp for the definitions.
 User can change this to their needs. We have also left some space for custom types.

 Special: (sensor 255 reserved for special commands)
 You can receive a node sketch name with MyMQTT/20/255/V_Sketch_name (or version with _version)

 To-do:
 Special commands : clear or set EEPROM Values, Send REBOOT and Receive reboot for MyMQTT itself.
 Be able to send ACK so client returns the data being sent.
 ... Please come with ideas!
 What to do with publish messages.

 Test in more MQTT clients, So far tested in openhab and MyMQTT for Android (Not my creation)
 - http://www.openhab.org/
 - https://play.google.com/store/apps/details?id=at.tripwire.mqtt.client&hl=en
 ... Please notify me if you use this broker with other software.


 * How to set-up Openhab and MQTTGateway:
 http://forum.mysensors.org/topic/303/mqtt-broker-gateway

 */

#include <SPI.h>
#include <MySensor.h>
#include "MyMQTTClient.h"
#include "PubSubClient.h"

#include <Ethernet.h>

/*
 * To configure MQTTClientGateway.ino to use an ENC28J60 based board include
 * 'UIPEthernet.h' (SPI.h required for MySensors anyway). The UIPEthernet-library can be downloaded
 * from: https://github.com/ntruchsess/arduino_uip
 */

//#include <UIPEthernet.h>
/*
 * To execute MQTTClientGateway.ino on Yun uncomment Bridge.h and YunClient.h.
 * Do not include Ethernet.h or SPI.h in this case.
 * On Yun there's no need to configure local_ip and mac in the sketch
 * as this is configured on the linux-side of Yun.
 */

//#include <Bridge.h>
//#include <YunClient.h>
// * Use this for IBOARD modded to use standard MISO/MOSI/SCK, see note *1 above!
/*
 #define RADIO_CE_PIN        3			// radio chip enable
 #define RADIO_SPI_SS_PIN    8			// radio SPI serial select
 #define RADIO_ERROR_LED_PIN A2  		// Error led pin
 #define RADIO_RX_LED_PIN    A1  		// Receive led pin
 #define RADIO_TX_LED_PIN    A0  		// the PCB, on board LED*/

// * Use this for default configured pro mini / nano etc :
///*
#define RADIO_CE_PIN        5		// radio chip enable
#define RADIO_SPI_SS_PIN    6		// radio SPI serial select
#define RADIO_ERROR_LED_PIN 7		// Error led pin
#define RADIO_RX_LED_PIN    8		// Receive led pin
#define RADIO_TX_LED_PIN    9		// the PCB, on board LED*/

//replace with ip of server you want to connect to, comment out if using 'remote_host'
uint8_t remote_ip[] =
  { 192, 168, 0, 1 };
//replace with hostname of server you want to connect to, comment out if using 'remote_ip'
//char* remote_ip = "server.local";
//replace with the port that your server is listening on
#define remote_port 1883
//replace with arduinos ip-address. Comment out if Ethernet-startup should use dhcp. Is ignored on Yun
uint8_t local_ip[] = {192,168,0,6};
//replace with ethernet shield mac. It's mandatory every device is assigned a unique mac. Is ignored on Yun
uint8_t mac[] =
  { 0x90, 0xA2, 0xDA, 0x0D, 0x07, 0x02 };

//////////////////////////////////////////////////////////////////

#if defined remote_ip && defined remote_host
#error "cannot define both remote_ip and remote_host at the same time!"
#endif

#ifdef _YUN_CLIENT_H_
YunClient ethClient;
#else
EthernetClient ethClient;
#endif

void
processMQTTMessages(char* topic, byte* payload, unsigned int length);

PubSubClient client(remote_ip, remote_port, processMQTTMessages, ethClient);
MyMQTTClient gw(client,RADIO_CE_PIN, RADIO_SPI_SS_PIN);

void
setup()
{
  Ethernet.begin(mac, local_ip);
  delay(1000);   // Wait for Ethernet to get configured.
  gw.begin(RF24_PA_LEVEL_GW, RF24_CHANNEL, RF24_DATARATE,
  RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN);
}

void
loop()
{
  if (!client.connected())
    {
      client.connect("MySensor");
      client.subscribe("MyMQTT/#");
    }
  client.loop();
  gw.processRadioMessage();
}

void
processMQTTMessages(char* topic, byte* payload, unsigned int length)
{
  gw.processMQTTMessage(topic, payload, length);
}
