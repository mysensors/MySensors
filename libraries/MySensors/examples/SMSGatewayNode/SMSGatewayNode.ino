/*
  SMS Gateway

  This node works as a SMS gateway, receiving and sending SMS messages at the request of other nodes
  It needs to have a fixed ID to be able to be addessed within a MySensors network.
  As it is easy to remember I chose the ID 123.

  Once the node is sent an initiating SMS, it will respond to the received calling number as it's "owner". This way you do not
  need to hardcode your number in the sketch. But to make it operational, you will need to send at least 1 SMS to it
  with the prededfined text MYPASSWORD (see below in the definitions, you set your own string).
  The node should respond with CONFIRMOWNER, you can set this string in de definitions below.
  The allow another phone to control the node and receive messages, the "owner" needs to release the node by sending a SMS with
  the text MYRELEASEWORD. Again you can set this string in the definitions below.
  The node will then again become ownerless and ignore all messages and SMS's.
  
  If the node has an owner and another phone sends a message to the node, it will inform the owner of a "take over attempt".

  You can sent V_TEXT to other nodes in your network by using the following format:

  "N021:xxx.any.25.characters.xxx"

  Anything sent after the "N021:" will be sent to the node with address 021. A V_TEXT has a maximum length of 25 charters. Adding the node address make the SMS length 30 charters.
  The SMS buffer is 31 characters long.

  A node address can have a value 000 (the controller) and any value between 001 and 254. Values higher then 254 are ignored and nothing will be done with the
  received SMS, apart from informing the controller that this SMS was received.
  
  Sending a SMS to the SMS Gateway with the address of the SMS Gateway itself will result in the SMS Gateway sending the same SMS back.
  So if you send the following SMS to your SMS Gateway:
  "N123:Are you there ?"
  You will receive a SMS with the content ""N123:Are you there ?"
  
  What is happening ?
  
  The SMS Gateway receives a SMS adressed to the node 123, the SMS Gateway sends a copy of the received SMS to the controller and sends a message with content "Are you there ?"
  to the node 123. Since node 123 is the SMS Gateway receiving a message, it will pass on this message as a SMS to the owner in the format specified earlier.
  So a text SMS is sent with content "N123:Are you there?"

  If a node sends a V_TEXT "Garagedoor is opened" to the SMS Gateway, the SMS gateway will pass this on to the owner's GSM in the format below:
  "N011:Garagedoor is opened"
  Address from the node sending the text is sent first (in this case "N011:"), and then the contents of the V_TEXT itself is appended ("Garagedoor is opened").

  You need a SIM800L module hooked to an atmega328 based MySensor node via softserial port. This sketch was made for my AC capable
  board wich allows use of a SIM800L module. More on that board on the forum at this location:
  http://forum.mysensors.org/topic/2374/50mm-x-50mm-board-with-different-powering-options/10
  or
  https://www.openhardware.io/view/11/ACDCBatteries-capable-atmega328p-board
  
  Nodes that want to inform an GSM owner via the SMS Gateway need to have the capacity to send a V_TEXT.
  Even nodes that sleep most of the time can send messages to any other node. You just need to define a V_TEXT message type.

  Nodes that need to react to SMS messages with instructions, need to be able to receive V_TEXT and need to be reachable (always on).
  So battery based nodes will not be the best candidates as actuators based on SMS.

  This example code is in the public domain.

  modified 10 oktober 2015  version 1.0
  modified 6 january 2016   version 1.1
  modified 15 january 2016  version 1.2

  by GertSanders on the MySensors forum

  Uses the Adafruit FONA library to drive the SIM800L, this need to be included in your library to compile this sketch.


  To do:  create a buffer structure to handle several incoming messages from nodes or phone.


*/

#define SKETCHNAME "SMSGatewayNode"
#define SKETCHVERSION "1.2"


// Enable debug prints to serial monitor

//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE

//#define MY_BAUD_RATE 57600

#define MY_NODE_ID 123
#define GATEWAY_ID 0

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// defines for sensors
//#define BATTERYLEVEL  0
#define SMSTEXT         1


// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE


// All includes
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_FONA.h>
#include <MySensors.h>

// Pin definitions and other defines
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

#define defaultledpin 8 // usually this is pin 13, but better to use another digital pin

// constants

#define MAXSMSLENGTH 30     // No need for longer messages from GSM's as V_TEXT is limiting what can be sent to the other nodes. Node addresses are maximum 000 -> 255
#define MAXMSGLENGTH 25     // limiting factor is the payload of V_TEXT, which is 25 chars. Nodes will never send you more then this in 1 message.
#define MAXNUMBERLENGTH 12  // based on "plus" sign, a country prefix and national number. Example in Belgium: +32 123 45 67
#define MAXNODEID 254       // highest value a node ID can have

// following strings need to be set by you, maximum length is 30 CHARS.
#define MYPASSWORD    "Password"       // replace this with your preferred passphrase to gain ownership
#define CONFIRMOWNER  "I am yours"     // replace this string if your partner is of the untrusting type ;-)
#define MYRELEASEWORD "I release you"  // replace this with your preferred release confirmation

// variables

boolean KnowMyOwner = false;
boolean MsgReceived = false;
uint16_t smslen = 0;
int8_t smsnum = 0;
int bufferindex = 0;

char smsbuffer[MAXSMSLENGTH + 1] = "";
char msgbuffer[MAXMSGLENGTH + 1] = "";
char nodemsgbuffer[MAXMSGLENGTH + 1] = "";
char nodeaddress[5];
char Numbers[11] = "0123456789";

char myOwnersNumber[MAXNUMBERLENGTH + 1] = "";
char ReceivedNumber[MAXNUMBERLENGTH + 1] = "";

// Instantiate objects
MyMessage SMSMsg(SMSTEXT, V_TEXT);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;







/////////////////////////////////////////////////////////////////////////
// supporting functions.
/////////////////////////////////////////////////////////////////////////
void blipled()
{
  digitalWrite(defaultledpin, HIGH);   // turn the LED on (HIGH is the voltage level)
  wait(3);              // wait for a second
  digitalWrite(defaultledpin, LOW);    // turn the LED off by making the voltage HIGH
  wait(100);              // wait for a second
  digitalWrite(defaultledpin, HIGH);   // turn the LED on (HIGH is the voltage level)
  wait(3);              // wait for a second
  digitalWrite(defaultledpin, LOW);    // turn the LED off by making the voltage HIGH
}



/////////////////////////////////////////////////////////////////////////
int GetNodeID()
{
  int TheAddress = -1;
  TheAddress = ((smsbuffer[1] - 48) * 100) + ((smsbuffer[2] - 48) * 10) + (smsbuffer[3] - 48);
  return TheAddress;
}



/////////////////////////////////////////////////////////////////////////
bool IsAddressingNode()
{
  if (strlen(smsbuffer) > 5)
  { // at least something to check
    if ((smsbuffer[0] == 'N') && (smsbuffer[4] == ':') && (strchr(Numbers, smsbuffer[1]) != NULL) && (strchr(Numbers, smsbuffer[2]) != NULL) && (strchr(Numbers, smsbuffer[3]) != NULL))
    {
#ifdef MY_DEBUG
      Serial.print("Message addressed to: ");
      Serial.println(GetNodeID());
#endif
      if (GetNodeID() <= MAXNODEID)
        return true;
      else
        return false;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}


/////////////////////////////////////////////////////////////////////////
char* GetMessageContent(char incomingsmsbuffer[MAXSMSLENGTH])
{
  strcpy(nodemsgbuffer, "");
  for (int i = 5; i < MAXSMSLENGTH; i++)
  {
    nodemsgbuffer[i - 5] = incomingsmsbuffer[i];
  }
  return nodemsgbuffer;
}


/////////////////////////////////////////////////////////////////////////
void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo(SKETCHNAME, SKETCHVERSION);
  wait(500);
  // node ID is 123
  present(SMSTEXT, S_INFO);
  wait(500);
}




/////////////////////////////////////////////////////////////////////////
void receive(const MyMessage & message)
{
#ifdef MY_DEBUG
  Serial.print(F("Incoming from sensor: "));
  Serial.print(message.sensor);
#endif



  if ((message.type == V_TEXT) and (!MsgReceived))
  {
    sprintf(smsbuffer, "N%d:", message.sender);
    strcat(smsbuffer, message.getString());
    strcpy(msgbuffer, message.getString());
    MsgReceived = true;

#ifdef MY_DEBUG
    Serial.print(F(", New text: "));
    Serial.print(message.getString());
#endif
    // send a copy of the received message to the controller
  }

#ifdef MY_DEBUG
  Serial.println();
#endif

}  // end: void receive()



/////////////////////////////////////////////////////////////////////////
void clearSMSmemory()
{
  int8_t smsnum = fona.getNumSMS();
  while (smsnum > 0)
  {
    fona.deleteSMS(smsnum);
    smsnum--;
  }
}



/////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board
/////////////////////////////////////////////////////////////////////////
void setup()
{
  KnowMyOwner = false;
  MsgReceived = false;
  SMSMsg.setDestination(GATEWAY_ID);

  // initialize digital LEDpin as an output.
  pinMode(defaultledpin, OUTPUT);
  digitalWrite(defaultledpin, HIGH);    // turn the LED on by making the voltage HIGH

#ifdef MY_DEBUG

  Serial.println(F("SIM800L basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));
  Serial.println(F("starting SIM800L"));

#endif

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial))
  {
#ifdef MY_DEBUG
    Serial.println(F("Couldn't find SIM800L"));
#endif
    while (1);
  }

#ifdef MY_DEBUG
  Serial.println(F("SIM800L is OK"));
#endif

  digitalWrite(defaultledpin, LOW);                   // turn the LED off by making the voltage LOW
  wait(250);                                   // wait for a bit
  digitalWrite(defaultledpin, HIGH);                    // turn the LED on by making the voltage HIGH

#ifdef MY_DEBUG
  // Print SIM card IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print(F("SIM card IMEI: ")); Serial.println(imei);
  }
#endif

  smsnum = fona.getNumSMS();

  if (smsnum < 0)
  {
#ifdef MY_DEBUG
    Serial.println(F("Could not read # SMS"));
#endif
  }
  else
  {
#ifdef MY_DEBUG
    Serial.print(smsnum);
    Serial.println(F(" SMS's on SIM card!"));
#endif
  }

  clearSMSmemory();

  smsnum = fona.getNumSMS();

  if (smsnum == 0)
  {
#ifdef MY_DEBUG
    Serial.println(F("SMS Memory cleared"));
#endif
  }

  digitalWrite(defaultledpin, LOW);                   // turn the LED off by making the voltage LOW

}

///////////////////////////////////////////////////////
// the loop function runs over and over again forever
///////////////////////////////////////////////////////
void loop()
{
  blipled();
  //  sensor.request(SMSTEXT, V_TEXT, 0);

  wait(500);              // wait for a second and proces messages

#ifdef MY_DEBUG
  Serial.print(F("Owner's number: "));
  Serial.println(myOwnersNumber);
  Serial.print(F("Last received number: "));
  Serial.println(ReceivedNumber);
#endif

  smsnum = fona.getNumSMS();

  if (smsnum < 0)
  {
#ifdef MY_DEBUG
    Serial.println(F("Could not read # SMS"));
#endif
  }

#ifdef MY_DEBUG
  Serial.println(F("Process incoming messages"));
#endif

  if (MsgReceived && KnowMyOwner)
  {
    // a node sent us V_TEXT and we know the owner, so we can send an SMS

    SMSMsg.setDestination(GATEWAY_ID);
    send(SMSMsg.set(smsbuffer));

    if (fona.sendSMS(myOwnersNumber, smsbuffer))
    {
      // SMS was sent to owner
      MsgReceived = false;
      strcpy(smsbuffer, "");
      strcpy(msgbuffer, "");
    }
    else
    {
      // We do not know the owner, then ignore this message, we know the owner but no message was received, so just to be safe:
      MsgReceived = false;
      strcpy(msgbuffer, "");
    }
  }


  if (smsnum > 0)
  { // we received an SMS and now need to check it

#ifdef MY_DEBUG
    Serial.print(smsnum);
    Serial.println(F(" SMS received"));
#endif

    if (! fona.readSMS(smsnum, smsbuffer, MAXSMSLENGTH, &smslen))
    { // could not read SMS

#ifdef MY_DEBUG
      Serial.println(F("Failed to get SMS!"));
#endif

    }
    else
    { // We could read the SMS, now retreive sms to smsbuffer and number of origin
      fona.getSMSSender(smsnum, ReceivedNumber, MAXNUMBERLENGTH);

#ifdef MY_DEBUG
      Serial.print(F("***** SMS #")); Serial.print(smsnum);
      Serial.print(F(" (")); Serial.print(smslen); Serial.println(F(") bytes *****"));
      Serial.println(smsbuffer);
      Serial.print(F("from: "));
      Serial.println(ReceivedNumber);
      Serial.println(F("***************************"));
      Serial.print(F("Owner is known: "));
      if (KnowMyOwner)
        Serial.println(F("yes"));
      else
        Serial.println(F("no"));
      Serial.println();
#endif

      if (!KnowMyOwner)
      { // no owner, so check request for ownership
        if (strcmp(smsbuffer, MYPASSWORD) == 0)
        { // valid question for ownership
          strcpy(smsbuffer, "OwnReq by: ");
          strcat(smsbuffer, ReceivedNumber);
          send(SMSMsg.set(smsbuffer));
          KnowMyOwner = true;
          strcpy(myOwnersNumber, ReceivedNumber);
          // from now on we can respond to owner

#ifdef MY_DEBUG
          Serial.print(F("Request for ownership by "));
          Serial.println(myOwnersNumber);
#endif

          strcpy(smsbuffer, CONFIRMOWNER);
          if (fona.sendSMS(myOwnersNumber, smsbuffer))
          { // send confirmation to new owner
            strcpy(smsbuffer, "Owner is informed.");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
            
#ifdef MY_DEBUG
            Serial.print(smsbuffer);
            Serial.print(": ");
            Serial.println(myOwnersNumber);
#endif
          }
          else
          { // could not send confirmation to new owner

            strcpy(smsbuffer, "Owner is not informed.");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));

          }



          strcpy(smsbuffer, "");
        }
        else
        { // no owner and no valid request, so ignore and log in controller
          strcpy(smsbuffer, "Attempt by: ");
          strcat(smsbuffer, ReceivedNumber);
          SMSMsg.setDestination(GATEWAY_ID);
          send(SMSMsg.set(smsbuffer));
          strcpy(smsbuffer, "");
        }

      }


      if (KnowMyOwner)
      { // now we can accept SMS messages but we need to check the origin of the SMS

        if (strcmp(myOwnersNumber, ReceivedNumber) == 0)
        { // this is the owner talking, so handle his SMS

#ifdef MY_DEBUG
          Serial.println();
          Serial.print(F("Owner is talking: "));
          Serial.println(smsbuffer);
          Serial.println();
#endif

          if (strcmp(smsbuffer, MYRELEASEWORD) == 0)
          {

            strcpy(smsbuffer, "Released by ");
            strcat(smsbuffer, ReceivedNumber);
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));

            strcpy(smsbuffer, "I am released.");
            if (fona.sendSMS(myOwnersNumber, smsbuffer))
            {
              strcpy(smsbuffer, "Owner is informed.");
              SMSMsg.setDestination(GATEWAY_ID);
              send(SMSMsg.set(smsbuffer));
            }
            else
            {
              strcpy(smsbuffer, "Owner is not informed.");
              SMSMsg.setDestination(GATEWAY_ID);
              send(SMSMsg.set(smsbuffer));
            }

            // clean up buffers, we are done and need to get ready for new owner request, ignore further messages
            KnowMyOwner = false;
            MsgReceived = false;
            strcpy(myOwnersNumber, "");
            strcpy(ReceivedNumber, "");
            strcpy(smsbuffer, "");
            strcpy(msgbuffer, "");
          }


          if (strcmp(smsbuffer, "Blip") == 0)
          { // handle the command to light the defaultled. This could be anything else of course
            
            digitalWrite(defaultledpin, HIGH);   // turn the LED on (HIGH is the voltage level)
            wait(2000);              // wait for two seconds
            digitalWrite(defaultledpin, LOW);    // turn the LED off by making the voltage LOW

            strcpy(smsbuffer, "R: Blip");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
            strcpy(smsbuffer, "");
          }


          // and here we put the code to pass the message on to another node
          // smsbuffer contains the first 30 characters of the SMS received.
          // if a message starts with Nxxx:1234567890123456789012345 then you need to pass on the content to node Nxxx
          // the node needs to be able to handle the incoming V_TEXT message

          if (IsAddressingNode()) // check if sms is in Node addressing format
          {
            int NodeID = GetNodeID();

#ifdef MY_DEBUG

            Serial.println();
            Serial.print("Message for node ");
            Serial.print(NodeID);
            Serial.print(" is treated: ");
            Serial.println(GetMessageContent(smsbuffer));
            Serial.println();

#endif

// send a copy of the incoming message to the controller
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
// send the stripped message to the addressed node
            SMSMsg.setDestination(NodeID);
            SMSMsg.set(GetMessageContent(smsbuffer));
            send(SMSMsg);

            strcpy(smsbuffer, "");
          }





          // finally we mention to the controller all the SMS messages we received and did not treat.

          if (strcmp(smsbuffer, "") != 0)
          { // so we still got something to do
#ifdef MY_DEBUG
            Serial.println(smsbuffer);
            Serial.println();
#endif
            //strcpy(smsbuffer, "Received crap from owner.");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
            strcpy(smsbuffer, "");
          }

        }
        else
        { // not the owner trying to get access
          strcpy(smsbuffer, "Hack by:");
          strcat(smsbuffer, ReceivedNumber);
          SMSMsg.setDestination(GATEWAY_ID);
          send(SMSMsg.set(smsbuffer));
#ifdef MY_DEBUG
          Serial.print(F("Hijack attempt by: "));
          Serial.println(ReceivedNumber);
#else
          if (fona.sendSMS(myOwnersNumber, smsbuffer))
          {
            strcpy(smsbuffer, "Owner is informed.");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
          }
          else
          {
            strcpy(smsbuffer, "Owner could not be informed.");
            SMSMsg.setDestination(GATEWAY_ID);
            send(SMSMsg.set(smsbuffer));
          }
#endif
        }

      }

    }

    clearSMSmemory();

  }

}







