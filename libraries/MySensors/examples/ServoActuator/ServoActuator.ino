// Example showing how to create an atuator for a servo.
// Connect red to +5V, Black or brown to GND and the last cable to Digital pin 3.
// The servo consumes much power and should probably have its own powersource.'
// The arduino might spontanally restart if too much power is used (happend
// to me when servo tried to pass the extreme positions = full load).
// Contribution by: Derek Macias


#include <MySensor.h>
#include <SPI.h>
#include <Servo.h> 

#define SERVO_DIGITAL_OUT_PIN 3
#define SERVO_MIN 0 // Fine tune your servos min. 0-180
#define SERVO_MAX 180  // Fine tune your servos max. 0-180
#define DETACH_DELAY 900 // Tune this to let your movement finish before detaching the servo
#define CHILD_ID 10   // Id of the sensor child

MySensor gw;
MyMessage msg(CHILD_ID, V_DIMMER);
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created Sensor gw(9,10);
unsigned long timeOfLastChange = 0;
bool attachedServo = false;
            
void setup() 
{ 
  // Attach method for incoming messages
  gw.begin(incomingMessage);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Servo", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_COVER);

  // Request last servo state at startup
  gw.request(CHILD_ID, V_DIMMER);
} 

void loop() 
{ 
  gw.process();
  if (attachedServo && millis() - timeOfLastChange > DETACH_DELAY) {
     myservo.detach();
     attachedServo = false;
  }
} 

void incomingMessage(const MyMessage &message) {
  myservo.attach(SERVO_DIGITAL_OUT_PIN);   
  attachedServo = true;
  if (message.type==V_DIMMER) { // This could be M_ACK_VARIABLE or M_SET_VARIABLE
     int val = message.getInt();
     myservo.write(SERVO_MAX + (SERVO_MIN-SERVO_MAX)/100 * val); // sets the servo position 0-180
     // Write some debug info
     Serial.print("Servo changed. new state: ");
     Serial.println(val);
   } else if (message.type==V_UP) {
     Serial.println("Servo UP command");
     myservo.write(SERVO_MIN);
     gw.send(msg.set(100));
   } else if (message.type==V_DOWN) {
     Serial.println("Servo DOWN command");
     myservo.write(SERVO_MAX); 
     gw.send(msg.set(0));
   } else if (message.type==V_STOP) {
     Serial.println("Servo STOP command");
     myservo.detach();
     attachedServo = false;

   }
   timeOfLastChange = millis();
}



