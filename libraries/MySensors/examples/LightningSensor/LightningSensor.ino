// This sketch is to integrate the Playing With Fusion AXS3935 Lightning Sensor Breakout Board
// with the MySensors environment and is based on a sketch provided by Playing With Fusion
// http://playingwithfusion.com/productview.php?pdid=22&catid=1001
//

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>
// include Playing With Fusion AXS3935 libraries
#include <PWFusion_AS3935.h>

// setup CS pins used for the connection with the lightning sensor
// other connections are controlled by the SPI library)
#define CS_PIN 8
#define SI_PIN 7
#define IRQ_PIN 3
volatile int8_t AS3935_ISR_Trig = 0;

// #defines
#define AS3935_INDOORS       1
#define AS3935_OUTDOORS      0
#define AS3935_DIST_DIS      0
#define AS3935_DIST_EN       1
#define AS3935_CAPACITANCE   96      // <-- SET THIS VALUE TO THE NUMBER LISTED ON YOUR BOARD 
// prototypes
void AS3935_ISR();

PWF_AS3935  lightning0(CS_PIN, IRQ_PIN, SI_PIN);

#define CHILD_ID_DISTANCE 1
#define CHILD_ID_INTENSITY 2
MyMessage msgDist(CHILD_ID_DISTANCE, V_DISTANCE);
MyMessage msgInt(CHILD_ID_INTENSITY, V_VAR1);

void setup()  
{ 
  Serial.println("Playing With Fusion: AS3935 Lightning Sensor, SEN-39001");
  Serial.println("beginning boot procedure....");
  
  // setup for the the SPI library:
  SPI.begin();                            // begin SPI
  SPI.setClockDivider(SPI_CLOCK_DIV4);    // SPI speed to SPI_CLOCK_DIV16/1MHz (max 2MHz, NEVER 500kHz!)
  SPI.setDataMode(SPI_MODE1);             // MAX31855 is a Mode 1 device
                                          //    --> clock starts low, read on rising edge
  SPI.setBitOrder(MSBFIRST);              // data sent to chip MSb first 
  
  lightning0.AS3935_DefInit();            // set registers to default  
  // now update sensor cal for your application and power up chip
  lightning0.AS3935_ManualCal(AS3935_CAPACITANCE, AS3935_OUTDOORS, AS3935_DIST_EN);
                  // AS3935_ManualCal Parameters:
                  //   --> capacitance, in pF (marked on package)
                  //   --> indoors/outdoors (AS3935_INDOORS:0 / AS3935_OUTDOORS:1)
                  //   --> disturbers (AS3935_DIST_EN:1 / AS3935_DIST_DIS:2)
                  // function also powers up the chip
                  
  // enable interrupt (hook IRQ pin to Arduino Uno/Mega interrupt input: 1 -> pin 3 )
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), AS3935_ISR, RISING);
  // dump the registry data to the serial port for troubleshooting purposes
  lightning0.AS3935_PrintAllRegs();
  
  // delay execution to allow chip to stabilize.
  delay(1000);

}

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Lightning Sensor", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_DISTANCE, S_DISTANCE);
  present(CHILD_ID_INTENSITY, S_CUSTOM);
}

void loop()      
{     

  // This program only handles an AS3935 lightning sensor. It does nothing until 
  // an interrupt is detected on the IRQ pin.
  while(0 == AS3935_ISR_Trig){}
 
  // reset interrupt flag
  AS3935_ISR_Trig = 0;
  
  // now get interrupt source
  uint8_t int_src = lightning0.AS3935_GetInterruptSrc();
  if(0 == int_src)
  {
    Serial.println("Unknown interrupt source");
  }
  else if(1 == int_src)
  {
    uint8_t lightning_dist_km = lightning0.AS3935_GetLightningDistKm();
    uint32_t lightning_intensity = lightning0.AS3935_GetStrikeEnergyRaw();

    Serial.print("Lightning detected! Distance to strike: ");
    Serial.print(lightning_dist_km);
    Serial.println(" kilometers");
    Serial.print("Lightning detected! Lightning Intensity: ");
    Serial.println(lightning_intensity);
    send(msgDist.set(lightning_dist_km));
    send(msgInt.set(lightning_intensity));
  }
  else if(2 == int_src)
  {
    Serial.println("Disturber detected");
  }
  else if(3 == int_src)
  {
    Serial.println("Noise level too high");
  }
}

// this is irq handler for AS3935 interrupts, has to return void and take no arguments
// always make code in interrupt handlers fast and short
void AS3935_ISR()
{
  AS3935_ISR_Trig = 1;
}
