/* 				MyMQTT Client Gateway 0.1b
 Created by Norbert Truchsess <norbert.truchsess@t-online.de>
 Based on MyMQTT-broker gateway created by Daniel Wiegert <daniel.wiegert@gmail.com>
 Based on MySensors Ethernet Gateway by Henrik Ekblad <henrik.ekblad@gmail.com>
 http://www.mysensors.org
 
 Changes by Thomas Krebs <thkrebs@gmx.de>
 - Add signing support from MySensors 1.5 and update for MySensors 1.5;
 - Restructured code back to a C like implementation following the existing MQTTGateway
 Requires MySensors lib 1.5
 
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
   * Address-layout is : [MQTT_BROKER_PREFIX]/[NodeID]/[SensorID]/V_[SensorType][/set | /get]
 NodeID and SensorID is uint8 (0-255) number.
 Segment after SensorID is translation of the sensor type, look inside MyMQTT.cpp for the definitions.
 User can change this to their needs. We have also left some space for custom types.
 Special: (sensor 255 reserved for special commands)
 
 The last seqment is a command to to be send down to the sensor. The gateway is subscribed to topics which have 
 a command present! This is to avoid that a publish by the gateway will end up in the gateway via it subsription. 
 You can receive a node sketch name with MyMQTT/20/255/V_Sketch_name (or version with _version)
 
 So far tested in openhab and MyMQTT for Android (Not my creation)
 - http://www.openhab.org/
 * How to set-up Openhab and MQTTGateway:
 http://forum.mysensors.org/topic/303/mqtt-broker-gateway

 */

#include <SPI.h>
#include <MySensor.h>
#include <MyTransport.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <DigitalIO.h>
#include <MsTimer2.h>
#include <Time.h>

#include "MyMQTTClient.h"

#ifdef MY_SIGNING_FEATURE
#include <MySigningNone.h>
#include <MySigningAtsha204Soft.h>
#include <MySigningAtsha204.h>
#endif

//#define DSRTC
#ifdef DSRTC
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#endif

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
//

//#define RADIO_CE_PIN        5		// radio chip enable
//#define RADIO_SPI_SS_PIN    6		// radio SPI serial select
//#define RADIO_ERROR_LED_PIN 7		// Error led pin
//#define RADIO_RX_LED_PIN    8		// Receive led pin
//#define RADIO_TX_LED_PIN    9		// the PCB, on board LED*/

// CE_PIN and SPI_SS_PIN for Mega
#define RADIO_CE_PIN        48			// radio chip enable
#define RADIO_SPI_SS_PIN    49			// radio SPI serial select
#define RADIO_ERROR_LED_PIN A2  		// Error led pin
#define RADIO_RX_LED_PIN    A1  		// Receive led pin
#define RADIO_TX_LED_PIN    A0  		// the PCB, on board LED*/

//replace with ip of server you want to connect to, comment out if using 'remote_host'
uint8_t remote_ip[] =  { 192, 168, 178, 74 };  // Mosquitto broker

//replace with hostname of server you want to connect to, comment out if using 'remote_ip'
//char* remote_ip = "server.local";
//replace with the port that your server is listening on
#define remote_port 1883
//replace with arduinos ip-address. Comment out if Ethernet-startup should use dhcp. Is ignored on Yun
uint8_t local_ip[] = {192, 168, 178, 11};
//replace with ethernet shield mac. It's mandatory every device is assigned a unique mac. Is ignored on Yun
uint8_t mac[] = { 0xA2, 0xAE, 0xAD, 0xA0, 0xA0, 0xA2 };

#if defined remote_ip && defined remote_host
#error "cannot define both remote_ip and remote_host at the same time!"
#endif

#ifdef _YUN_CLIENT_H_
YunClient ethClient;
#else
EthernetClient ethClient;
#endif

////////////////////////////////////////////////////////////////
// NRFRF24L01 radio driver (set low transmit power by default)
MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);
//MyTransportRFM69 transport;

// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
//MySigningNone signer;
//MySigningAtsha204Soft signer(true);
//MySigningAtsha204 signer;

// Hardware profile
MyHwATMega328 hw;

MyMessage msg;
char convBuf[MAX_PAYLOAD * 2 + 1];
uint8_t buffsize;
char buffer[MQTT_MAX_PACKET_SIZE];
char user[sizeof(MQTT_USER)];                  // introduced the bufs to get rid of compiler warnings
char pwd[sizeof(MQTT_PWD)];                    // see above
char connID[sizeof(MQTT_CONN_ID)];             // see above
char mqttTopicMask[sizeof(MQTT_TOPIC_MASK)];   // see above

////////////////////////////////////////////////////////////////

volatile uint8_t countRx;
volatile uint8_t countTx;
volatile uint8_t countErr;


////////////////////////////////////////////////////////////////

void processMQTTMessages(char* topic, byte* payload, unsigned int length);
PubSubClient client(remote_ip, remote_port, processMQTTMessages, ethClient);

////////////////////////////////////////////////////////////////


// Declare and initialize MySensor instance
// Construct MyMQTTClient (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h, if signing
// feature not to be used, uncomment)
// To use LEDs blinking, uncomment WITH_LEDS_BLINKING in MyConfig.h
MySensor gw(transport, hw
#ifdef MY_SIGNING_FEATURE
            , signer
#endif
#ifdef WITH_LEDS_BLINKING
            , RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN
#endif
           );

/*
 * setup
 */
void setup()
{
  strcpy(connID,MQTT_CONN_ID);
  strcpy(mqttTopicMask,MQTT_TOPIC_MASK);
  
  countRx = 0;
  countTx = 0;
  countErr = 0;

  Ethernet.begin(mac, local_ip);
  //Bridge.begin();
  delay(1000);   // Wait for Ethernet to get configured.

  begin();
}

/*
 * loop
 */
void loop()
{
  if (!client.connected())
  {
    boolean ret;
#ifdef MQTT_AUTH_REQUIRED
    strcpy(user, MQTT_USER);
    strcpy(pwd, MQTT_PWD);
    ret = client.connect(connID, user,pwd);
#else
    ret = client.connect(connID);
#endif
    if (ret) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.println("Connection to MQTT broker failed");
    }
    client.subscribe(mqttTopicMask);
  }
  client.loop();
  gw.process();
}

/*
 * processRadioMessage
 *
 * Receives radio message, parses it and forwards it to the MQTT broker
 */
void processRadioMessage(const MyMessage &message)
{ 
    rxBlink(1);
    sendMQTT(message);
}

/*
 * sendMQTT
 * Handles processing of radio messages and eventually publishes it to the MQTT broker
 */
void sendMQTT(const MyMessage &inMsg)
{
  MyMessage msg = inMsg;
  buffsize = 0;
  if (!client.connected())
    return;			//We have no connections - return
  if (msg.isAck())
  {
#ifdef DEBUG
    Serial.println("msg is ack!");
#endif
    if (msg.sender == 255 && mGetCommand(msg) == C_INTERNAL
        && msg.type == I_ID_REQUEST)
    {
      // TODO: sending ACK request on id_response fucks node up. doesn't work.
      // The idea was to confirm id and save to EEPROM_LATEST_NODE_ADDRESS.
    }
  }
  else
  {
    // we have to check every message if its a newly assigned id or not.
    // Ack on I_ID_RESPONSE does not work, and checking on C_PRESENTATION isn't reliable.
    uint8_t newNodeID = gw.loadState(EEPROM_LATEST_NODE_ADDRESS) + 1;
    if (newNodeID <= MQTT_FIRST_SENSORID)
      newNodeID = MQTT_FIRST_SENSORID;
    if (msg.sender == newNodeID)
    {
      gw.saveState(EEPROM_LATEST_NODE_ADDRESS, newNodeID);
    }
    if (mGetCommand(msg) == C_INTERNAL)
    {
      if (msg.type == I_CONFIG)
      {
        txBlink(1);
        if (!gw.sendRoute(
              build(msg, GATEWAY_ADDRESS, msg.sender, 255, C_INTERNAL,
                    I_CONFIG, 0).set("M")))
          errBlink(1);
      }
      else if (msg.type == I_TIME)
      {
#ifdef DEBUG
        Serial.println("I_TIME requested!");
#endif
        txBlink(1);
        if (!gw.sendRoute(
              build(msg, GATEWAY_ADDRESS, msg.sender, 255, C_INTERNAL,
                    I_TIME, 0).set(now())))
          errBlink(1);
      }
      else if (msg.type == I_ID_REQUEST && msg.sender == 255)
      {
        uint8_t newNodeID = gw.loadState(EEPROM_LATEST_NODE_ADDRESS) + 1;
        if (newNodeID <= MQTT_FIRST_SENSORID)
          newNodeID = MQTT_FIRST_SENSORID;
        if (newNodeID >= MQTT_LAST_SENSORID)
          return; // Sorry no more id's left :(
        txBlink(1);
        if (!gw.sendRoute(
              build(msg, GATEWAY_ADDRESS, msg.sender, 255, C_INTERNAL,
                    I_ID_RESPONSE, 0).set(newNodeID)))
          errBlink(1);
      }
      else if (msg.type == I_BATTERY_LEVEL)
      {
        strcpy_P(buffer, mqtt_prefix);
        buffsize += strlen_P(mqtt_prefix);
        buffsize += sprintf(&buffer[buffsize], "/%i/255/BATTERY_LEVEL", msg.sender );
        msg.getString(convBuf);
#ifdef DEBUG
        Serial.print("publish: ");
        Serial.print((char*) buffer);
        Serial.print(" ");
        Serial.println((char*) convBuf);
#endif
        client.publish(buffer, convBuf);
      }
      else if (msg.type == I_SKETCH_NAME)
      {
        strcpy_P(buffer, mqtt_prefix);
        buffsize += strlen_P(mqtt_prefix);
        buffsize += sprintf(&buffer[buffsize], "/%i/255/SKETCH_NAME", msg.sender );
        msg.getString(convBuf);
#ifdef DEBUG
        Serial.print("publish: ");
        Serial.print((char*) buffer);
        Serial.print(" ");
        Serial.println((char*) convBuf);
#endif
        client.publish(buffer, convBuf);
      }
      else if (msg.type == I_SKETCH_VERSION)
      {
        strcpy_P(buffer, mqtt_prefix);
        buffsize += strlen_P(mqtt_prefix);
        buffsize += sprintf(&buffer[buffsize], "/%i/255/SKETCH_VERSION", msg.sender );
        msg.getString(convBuf);
#ifdef DEBUG
        Serial.print("publish: ");
        Serial.print((char*) buffer);
        Serial.print(" ");
        Serial.println((char*) convBuf);
#endif
        client.publish(buffer, convBuf);
      }

    }
    else if (mGetCommand(msg) != 0)
    {
      if (mGetCommand(msg) == 3)
        msg.type = msg.type + (S_FIRSTCUSTOM - 10);	//Special message

      if (msg.type > VAR_TOTAL)
        msg.type = VAR_TOTAL;		// If type > defined types set to unknown.
      strcpy_P(buffer, mqtt_prefix);
      buffsize += strlen_P(mqtt_prefix);
      buffsize += sprintf(&buffer[buffsize], "/%i/%i/V_%s", msg.sender,
                          msg.sensor, getType(convBuf, &VAR_Type[msg.type]));
      msg.getString(convBuf);
#ifdef DEBUG
      Serial.print("publish: ");
      Serial.print((char*) buffer);
      Serial.print(" ");
      Serial.println((char*) convBuf);
#endif
      client.publish(buffer, convBuf);
    }
  }
}

/*
 * build
 * Constructs a radio message
 */
inline MyMessage& build(MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor,
                        uint8_t command, uint8_t type, bool enableAck)
{
  msg.destination = destination;
  msg.sender = sender;
  msg.sensor = sensor;
  msg.type = type;
  mSetCommand(msg, command);
  mSetRequestAck(msg, enableAck);
  mSetAck(msg, false);
  return msg;
}

/*
 * getType
 */
char *getType(char *b, const char **index)
{
  char *q = b;
  char *p = (char *) pgm_read_word(index);
  while (*q++ = pgm_read_byte(p++))
    ;
  *q = 0;
  return b;
}

/*
 * begin
 * wraps MySensors begin method; setup of RTC and led timers interrupt
 */
void begin()
{
#ifdef DEBUG
  Serial.begin(BAUD_RATE);
#endif
#ifdef DSRTC
  // Attach RTC
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(60);
#endif

  gw.begin(processRadioMessage, 0, true, 0);

  MsTimer2::set(200, ledTimersInterrupt);
  MsTimer2::start();

#ifdef DEBUG
  Serial.print(getType(convBuf, &VAR_Type[S_FIRSTCUSTOM]));
#endif
}

/*
 * processMQTTMessages
 * message handler for the PubSubClient
 */
void processMQTTMessages(char* topic, byte* payload, unsigned int length)
{
  processMQTTMessage(topic, payload, length);
}

/*
 * processMQTTMessage
 * processes MQTT messages, parses the topic, extracts radio address out of topic and sends them
 * to the respective radio
 */
void processMQTTMessage(char* topic, byte* payload,
                        unsigned int length)
{
  char *str, *p;
  uint8_t i = 0;
  buffer[0] = 0;
  buffsize = 0;
  int8_t cmd = -1;

  for (str = strtok_r(topic, "/", &p); str && i < 5;
       str = strtok_r(NULL, "/", &p))
  {
    switch (i)
    {
      case 0:
        {
          if (strcmp_P(str, mqtt_prefix) != 0)
          { //look for MQTT_PREFIX
            return;			//Message not for us or malformatted!
          }
          break;
        }
      case 1:
        {
          msg.destination = atoi(str);	//NodeID
          break;
        }
      case 2:
        {
          msg.sensor = atoi(str);		//SensorID
          break;
        }
      case 3:
        {
          char match = 0;			//SensorType
          //strcpy(str,(char*)&str[2]);  //Strip VAR_

          for (uint8_t j = 0;
               strcpy_P(convBuf, (char*) pgm_read_word(&(VAR_Type[j]))); j++)
          {
            if (strcmp((char*) &str[2], convBuf) == 0)
            { //Strip VAR_ and compare
              match = j;
              break;
            }
            if (j >= VAR_TOTAL)
            { // No match found!
              match = VAR_TOTAL;	// last item.
              break;
            }
          }
          msg.type = match;
          break;
        }
       case 4:
          {
            // support the command get and set; get will be mapped to a C_REQ and set to C_SET
            if (strcmp(str,MQTT_CMD_SET) == 0)
            {
              cmd = C_SET;
            }
            else if (strcmp(str,MQTT_CMD_GET) == 0) {
              cmd = C_REQ;
            } 
            else {
#ifdef DEBUG
              Serial.print("Received unsupported command - ignore: ");
              Serial.println(str);
#endif              
            }
          } 
    }
    i++;
  }						//Check if packge has payload

  if (cmd != -1) {
    char* ca;
    ca = (char *)payload;
    ca += length;
    *ca = '\0';

    msg.set((const char*)payload);			//Payload

    txBlink(1);
    // inject time
    if ((msg.destination == 0) && (msg.sensor == 199)) {
      unsigned long epoch = atol((char*)payload);
      if (epoch > 10000) {
#ifdef DSRTC
        RTC.set(epoch); // this sets the RTC to the time from controller - which we do want periodically
#endif
        setTime(epoch);
      }
#ifdef DEBUG
      Serial.print("Time received ");
      Serial.println(epoch);
#endif
    }
    //
    if (!gw.sendRoute(
          build(msg, GATEWAY_ADDRESS, msg.destination, msg.sensor, C_SET, msg.type,
                0)))
      errBlink(1);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Led handling
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * ledTimersIntterupt
 */
void ledTimersInterrupt() {
  if (countRx && countRx != 255) {
    // switch led on
    digitalWrite(RADIO_RX_LED_PIN, LOW);
  } else if (!countRx) {
    // switching off
    digitalWrite(RADIO_RX_LED_PIN, HIGH);
  }
  if (countRx != 255) {
    countRx--;
  }

  if (countTx && countTx != 255) {
    // switch led on
    digitalWrite(RADIO_TX_LED_PIN, LOW);
  } else if (!countTx) {
    // switching off
    digitalWrite(RADIO_TX_LED_PIN, HIGH);
  }
  if (countTx != 255) {
    countTx--;
  }

  if (countErr && countErr != 255) {
    // switch led on
    digitalWrite(RADIO_ERROR_LED_PIN, LOW);
  } else if (!countErr) {
    // switching off
    digitalWrite(RADIO_ERROR_LED_PIN, HIGH);
  }
  if (countErr != 255) {
    countErr--;
  }
}


void rxBlink(uint8_t cnt)
{
  if (countRx == 255)
  {
    countRx = cnt;
  }
}

void txBlink(uint8_t cnt)
{
  if (countTx == 255)
  {
    countTx = cnt;
  }
}

void errBlink(uint8_t cnt)
{
  if (countErr == 255)
  {
    countErr = cnt;
  }
}


