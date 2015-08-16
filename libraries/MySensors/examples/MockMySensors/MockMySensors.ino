/*
* FakeMySensors
*
* This skecth is intended to crate fake sensors which register and respond to the controller
* 
* Barduino, GizMoCuz 2015
*/

#include <MySensor.h>  
#include <SPI.h>

// SPI Pins
#define CE_PIN 9
#define CS_PIN 10

// Wait times
#define LONG_WAIT 500
#define SHORT_WAIT 50

#define SKETCH_NAME "FakeMySensors "
#define SKETCH_VERSION "v0.3"

////#define ID_S_ARDUINO_NODE            //auto defined in initialization
////#define ID_S_ARDUINO_REPEATER_NODE   //auto defined in initialization 

//#define ID_S_DOOR                     1
//#define ID_S_MOTION                   2
//#define ID_S_SMOKE                    3
//#define ID_S_LIGHT                    4
//#define ID_S_DIMMER                   5
//#define ID_S_COVER                    6
//#define ID_S_TEMP                     7
//#define ID_S_HUM                      8
//#define ID_S_BARO                     9
//#define ID_S_WIND                     10
//#define ID_S_RAIN                      11
//#define ID_S_UV                      12
//#define ID_S_WEIGHT                  13 
//#define ID_S_POWER                   14
//#define ID_S_HEATER                  15  <<-- not correctly implemented (1.5 API)
//#define ID_S_DISTANCE                16
//#define ID_S_LIGHT_LEVEL             17 
//#define ID_S_LOCK                    18
//#define ID_S_IR                      19
//#define ID_S_WATER                   20 
//#define ID_S_AIR_QUALITY             21 
//#define ID_S_DUST                    22
//#define ID_S_SCENE_CONTROLLER        23
//#define ID_S_CUSTOM                  24

// Global Vars
unsigned long SLEEP_TIME = 12000; // Sleep time between reads (in milliseconds)
boolean metric = true;
long randNumber;

// Instanciate MySersors Gateway
MySensor gw;

//Instanciate Messages objects

#ifdef ID_S_DOOR
  MyMessage msg_S_DOOR(ID_S_DOOR,V_TRIPPED);
#endif

#ifdef ID_S_MOTION
  MyMessage msg_S_MOTION_A(ID_S_MOTION,V_ARMED);
  MyMessage msg_S_MOTION_T(ID_S_MOTION,V_TRIPPED);
#endif

#ifdef ID_S_SMOKE
  MyMessage msg_S_SMOKE(ID_S_SMOKE,V_TRIPPED);
#endif

#ifdef ID_S_LIGHT
  MyMessage msg_S_LIGHT(ID_S_LIGHT,V_LIGHT);
#endif

#ifdef ID_S_DIMMER
  MyMessage msg_S_DIMMER(ID_S_DIMMER,V_DIMMER);
#endif

#ifdef ID_S_COVER
  MyMessage msg_S_COVER_U(ID_S_COVER,V_UP);
  MyMessage msg_S_COVER_D(ID_S_COVER,V_DOWN);
  MyMessage msg_S_COVER_S(ID_S_COVER,V_STOP);
#endif

#ifdef ID_S_TEMP
  MyMessage msg_S_TEMP(ID_S_TEMP,V_TEMP);
#endif

#ifdef ID_S_HUM
  MyMessage msg_S_HUM(ID_S_HUM,V_HUM);
#endif

#ifdef ID_S_BARO
  MyMessage msg_S_BARO_P(ID_S_BARO,V_PRESSURE);
  MyMessage msg_S_BARO_F(ID_S_BARO,V_FORECAST);
#endif

#ifdef ID_S_WIND
  MyMessage msg_S_WIND_S(ID_S_WIND,V_WIND);
  MyMessage msg_S_WIND_G(ID_S_WIND,V_GUST);
  MyMessage msg_S_WIND_D(ID_S_WIND,V_DIRECTION);
#endif

#ifdef ID_S_RAIN
  MyMessage msg_S_RAIN_A(ID_S_RAIN,V_RAIN);
  MyMessage msg_S_RAIN_R(ID_S_RAIN,V_RAINRATE);
#endif

#ifdef ID_S_UV
  MyMessage msg_S_UV(ID_S_UV,V_UV);
#endif

#ifdef ID_S_WEIGHT
  MyMessage msg_S_WEIGHT(ID_S_WEIGHT,V_WEIGHT);
#endif

#ifdef ID_S_POWER
  MyMessage msg_S_POWER_I(ID_S_POWER,V_IMPEDANCE);
  MyMessage msg_S_POWER_W(ID_S_POWER,V_WATT);
  MyMessage msg_S_POWER_K(ID_S_POWER,V_KWH);
  MyMessage msg_S_POWER_V(ID_S_POWER,V_VOLTAGE);
  MyMessage msg_S_POWER_C(ID_S_POWER,V_CURRENT);
#endif

#ifdef ID_S_HEATER
  float current_set_point=21.5;
  MyMessage msg_S_HEATER_SP(ID_S_HEATER,V_HVAC_SETPOINT_HEAT);
  MyMessage msg_S_HEATER_F(ID_S_HEATER,V_HVAC_SETPOINT_HEAT);
  MyMessage msg_S_HEATER_T(ID_S_HEATER,V_TEMP);
#endif

#ifdef ID_S_DISTANCE
  MyMessage msg_S_DISTANCE(ID_S_DISTANCE,V_DISTANCE);
#endif

#ifdef ID_S_LIGHT_LEVEL
  MyMessage msg_S_LIGHT_LEVEL(ID_S_LIGHT_LEVEL,V_LIGHT_LEVEL);
#endif

#ifdef ID_S_LOCK
  MyMessage msg_S_LOCK(ID_S_LOCK,V_LOCK_STATUS);
#endif

#ifdef ID_S_IR
  MyMessage msg_S_IR_S(ID_S_IR,V_IR_SEND);
  MyMessage msg_S_IR_R(ID_S_IR,V_IR_RECEIVE);
#endif

#ifdef ID_S_WATER
  MyMessage msg_S_WATER_F(ID_S_WATER,V_FLOW);
  MyMessage msg_S_WATER_V(ID_S_WATER,V_VOLUME);
#endif

#ifdef ID_S_AIR_QUALITY
  MyMessage msg_S_AIR_QUALITY(ID_S_AIR_QUALITY,V_LEVEL);
#endif

#ifdef ID_S_DUST
  MyMessage msg_S_DUST(ID_S_DUST,V_LEVEL);
#endif

#ifdef ID_S_SCENE_CONTROLLER
  MyMessage msg_S_SCENE_CONTROLLER_ON(ID_S_SCENE_CONTROLLER,V_SCENE_ON);
  MyMessage msg_S_SCENE_CONTROLLER_OF(ID_S_SCENE_CONTROLLER,V_SCENE_OFF);
#endif

#ifdef ID_S_CUSTOM
  MyMessage msg_S_CUSTOM_2(ID_S_CUSTOM,V_VAR2);
  MyMessage msg_S_CUSTOM_3(ID_S_CUSTOM,V_VAR3);
  MyMessage msg_S_CUSTOM_4(ID_S_CUSTOM,V_VAR4);
  MyMessage msg_S_CUSTOM_5(ID_S_CUSTOM,V_VAR5);
#endif




void setup()  
{ 
  // Random SEED
  randomSeed(analogRead(0));
  
  // Start the gateway
  gw.begin(incomingMessage,254);
  gw.wait(LONG_WAIT);
  Serial.println("GW Started");
  
  // Send the Sketch Version Information to the Gateway
  Serial.print("Send Sketch Info: ");
  gw.sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  Serial.print(SKETCH_NAME);
  Serial.println(SKETCH_VERSION);
  gw.wait(LONG_WAIT);
  
  // Get controller configuration
  Serial.print("Get Config: ");
  metric = gw.getConfig().isMetric;
  Serial.println(metric ? "Metric":"Imperial");
  gw.wait(LONG_WAIT);
  
  // Register all sensors to gw (they will be created as child devices)
  Serial.println("Presenting Nodes");
  Serial.println("________________");
  
  #ifdef ID_S_DOOR
    Serial.println("  S_DOOR");
    gw.present(ID_S_DOOR,S_DOOR);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_MOTION
    Serial.println("  S_MOTION");
    gw.present(ID_S_MOTION,S_MOTION);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_SMOKE
    Serial.println("  S_SMOKE");
    gw.present(ID_S_SMOKE,S_SMOKE);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_LIGHT
    Serial.println("  S_LIGHT");
    gw.present(ID_S_LIGHT,S_LIGHT);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_DIMMER
    Serial.println("  S_DIMMER");
    gw.present(ID_S_DIMMER,S_DIMMER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_COVER
    Serial.println("  S_COVER");
    gw.present(ID_S_COVER,S_COVER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_TEMP
    Serial.println("  S_TEMP");
    gw.present(ID_S_TEMP,S_TEMP);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_HUM
    Serial.println("  S_HUM");
    gw.present(ID_S_HUM,S_HUM);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_BARO
    Serial.println("  S_BARO");
    gw.present(ID_S_BARO,S_BARO);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_WIND
    Serial.println("  S_WIND");
    gw.present(ID_S_WIND,S_WIND);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_RAIN
    Serial.println("  S_RAIN");
    gw.present(ID_S_RAIN,S_RAIN);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_UV
    Serial.println("  S_UV");
    gw.present(ID_S_UV,S_UV);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_WEIGHT
    Serial.println("  S_WEIGHT"):
    gw.present(ID_S_WEIGHT,S_WEIGHT);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_POWER
    Serial.println("  S_POWER");
    gw.present(ID_S_POWER,S_POWER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_HEATER
    Serial.println("  S_HEATER");
    gw.present(ID_S_HEATER,S_HEATER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_DISTANCE
    Serial.println("  S_DISTANCE");
    gw.present(ID_S_DISTANCE,S_DISTANCE);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_LIGHT_LEVEL
    Serial.println("  S_LIGHT_LEVEL");
    gw.present(ID_S_LIGHT_LEVEL,S_LIGHT_LEVEL);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_LOCK
    Serial.println("  S_LOCK");
    gw.present(ID_S_LOCK,S_LOCK);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_IR
    Serial.println("  S_IR");
    gw.present(ID_S_IR,S_IR);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_WATER
    Serial.println("  S_WATER");
    gw.present(ID_S_WATER,S_WATER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_AIR_QUALITY
    Serial.println("  S_AIR_QUALITY");
    gw.present(ID_S_AIR_QUALITY,S_AIR_QUALITY);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_DUST  
    Serial.println("  S_DUST");
    gw.present(ID_S_DUST,S_DUST);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_SCENE_CONTROLLER
    Serial.println("  S_SCENE_CONTROLLER");
    gw.present(ID_S_SCENE_CONTROLLER,S_SCENE_CONTROLLER);
    gw.wait(SHORT_WAIT);
  #endif
  
  #ifdef ID_S_CUSTOM
    Serial.println("  S_CUSTOM");
    gw.present(ID_S_CUSTOM,S_CUSTOM);
    gw.wait(SHORT_WAIT);
  #endif



Serial.println("________________");

}

void loop()      
{ 
  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("#########################"); 
  randNumber=random(0,101);
  
  Serial.print("RandomNumber:");
  Serial.println(randNumber);
  // Send fake battery level
  Serial.println("Send Battery Level");
  gw.sendBatteryLevel(randNumber);
  gw.wait(LONG_WAIT);
  
  // Request time
  Serial.println("Request Time");
  gw.requestTime(receiveTime);
  gw.wait(LONG_WAIT);
  
  //Read Sensors
  #ifdef ID_S_DOOR 
    door(); 
  #endif
  
  #ifdef ID_S_MOTION
    motion();
  #endif
  
  #ifdef ID_S_SMOKE
    smoke();
  #endif
  
  #ifdef ID_S_LIGHT
    light();
  #endif
  
  #ifdef ID_S_DIMMER
    dimmer();
  #endif
  
  #ifdef ID_S_COVER
    cover();
  #endif
  
  #ifdef ID_S_TEMP
    temp();
  #endif
  
  #ifdef ID_S_HUM
    hum();
  #endif
  
  #ifdef ID_S_BARO
    baro();
  #endif
  
  #ifdef ID_S_WIND
    wind();
  #endif
  
  #ifdef ID_S_RAIN
    rain();
  #endif
  
  #ifdef ID_S_UV
    uv();
  #endif
   
  #ifdef ID_S_WEIGHT
    weight();
  #endif
  
  #ifdef ID_S_POWER
    power();
  #endif
  
  #ifdef ID_S_HEATER
    heater();
  #endif
  
  #ifdef ID_S_DISTANCE
    distance();
  #endif
  
  #ifdef ID_S_LIGHT_LEVEL
    light_level();
  #endif
  
  #ifdef ID_S_LOCK
    lock();
  #endif
  
  #ifdef ID_S_IR
    ir();
  #endif
  
  #ifdef ID_S_WATER
    water();
  #endif
  
  #ifdef ID_S_AIR_QUALITY
    air();
  #endif
  
  #ifdef ID_S_DUST
    dust();
  #endif
  
  #ifdef ID_S_SCENE_CONTROLLER
    scene();
  #endif
  
  #ifdef ID_S_CUSTOM
    custom();
  #endif
  
  gw.sendBatteryLevel(randNumber);
  gw.wait(SHORT_WAIT);
  gw.process();
  Serial.println("#########################");
  gw.wait(SLEEP_TIME); //sleep a bit
}

// This is called when a new time value was received
void receiveTime(unsigned long controllerTime) {

  Serial.print("Time value received: ");
  Serial.println(controllerTime);

}

#ifdef ID_S_DOOR
void door(){

  Serial.print("Door is: " );
  
  if (randNumber <= 50) {
    Serial.println("Open");
    gw.send(msg_S_DOOR.set(1));
  } else {
    Serial.println("Closed");
    gw.send(msg_S_DOOR.set(0));
  }
  
}
#endif

#ifdef ID_S_MOTION
void motion(){

  Serial.print("Motion is: " );
  
  if (randNumber <= 50) {
    Serial.println("Active");
    gw.send(msg_S_MOTION_T.set(1));
  } else {
    Serial.println("Quiet");
    gw.send(msg_S_MOTION_T.set(0));
  }
  
}
#endif

#ifdef ID_S_SMOKE
void smoke(){

  Serial.print("Smoke is: " );
  
  if (randNumber <= 50) {
    Serial.println("Active");
    gw.send(msg_S_SMOKE.set(1));
  } else {
    Serial.println("Quiet");
    gw.send(msg_S_SMOKE.set(0));
  }
  
}
#endif

#ifdef ID_S_LIGHT
void light(){

  bool isLightOn = gw.loadState(ID_S_LIGHT);
  
  Serial.print("Light is: " );
  
  if (isLightOn) {
    Serial.println("On");
    gw.send(msg_S_LIGHT.set(1));
  } else {
    Serial.println("Off");
    gw.send(msg_S_LIGHT.set(0));
  }
  
}
#endif

#ifdef ID_S_DIMMER
void dimmer(){

  int dimmerVal = gw.loadState(ID_S_DIMMER);
  
  Serial.print("Dimmer is set to: " );
  Serial.println(dimmerVal);
  
  gw.send(msg_S_DIMMER.set(dimmerVal));

}
#endif

#ifdef ID_S_COVER
void cover(){

  int coverVal = gw.loadState(ID_S_COVER);
  
  Serial.print("Cover is : " );
  
  switch (coverVal) {
    case 0:
      Serial.println("Opening");
      gw.send(msg_S_COVER_U.set(V_UP));
    break;
    case 1:
      Serial.println("Closing");
      gw.send(msg_S_COVER_D.set(V_DOWN));
    break;
    default:
      Serial.println("Idle");
      gw.send(msg_S_COVER_S.set(V_STOP));
  }
  
}
#endif

#ifdef ID_S_TEMP
void temp(){
  
  Serial.print("Temperature is: " );
  Serial.println(map(randNumber,1,100,0,45));
  
  gw.send(msg_S_TEMP.set(map(randNumber,1,100,0,45)));
  
}
#endif

#ifdef ID_S_HUM
void hum(){
  
  Serial.print("Humitidty is: " );
  Serial.println(randNumber);
  
  gw.send(msg_S_HUM.set(randNumber));
  
}
#endif

#ifdef ID_S_BARO
void baro(){
  
  const char *weather[] = {"stable","sunny","cloudy","unstable","thunderstorm","unknown"};
  long pressure = map(randNumber,1,100,900,1200);
  int forecast = map(randNumber,1,100,0,5);
  
  Serial.print("Atmosferic Pressure is: " );
  Serial.println(pressure);
  gw.send(msg_S_BARO_P.set(pressure));
  
  Serial.print("Weather forecast: " );
  Serial.println(weather[forecast]);
  gw.send(msg_S_BARO_F.set(weather[forecast]));
 
}
#endif

#ifdef ID_S_WIND
void wind(){
  
  Serial.print("Wind Speed is: " );
  Serial.println(randNumber);
  gw.send(msg_S_WIND_S.set(randNumber));
  
  Serial.print("Wind Gust is: " );
  Serial.println(randNumber+10);
  gw.send(msg_S_WIND_G.set(randNumber+10));
  
  Serial.print("Wind Direction is: " );
  Serial.println(map(randNumber,1,100,0,360));
  gw.send(msg_S_WIND_D.set(map(randNumber,1,100,0,360)));
  
}
#endif

#ifdef ID_S_RAIN
void rain(){
  
  Serial.print("Rain ammount  is: " );
  Serial.println(randNumber);
  
  gw.send(msg_S_RAIN_A.set(randNumber));
  
  Serial.print("Rain rate  is: " );
  Serial.println(randNumber/60);
  
  gw.send(msg_S_RAIN_R.set(randNumber/60,1));

}
#endif

#ifdef ID_S_UV
void uv(){
  
  Serial.print("Ultra Violet level is: " );
  Serial.println(map(randNumber,1,100,0,15));
  
  gw.send(msg_S_UV.set(map(randNumber,1,100,0,15)));

}
#endif

#ifdef ID_S_WEIGHT
void weight(){
  
  Serial.print("Weight is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_WEIGHT.set(map(randNumber,1,100,0,150)));

}
#endif

#ifdef ID_S_POWER
void power(){

  Serial.print("Impedance is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_POWER_I.set(map(randNumber,1,100,0,150)));

  Serial.print("Watt is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_POWER_W.set(map(randNumber,1,100,0,150)));

  Serial.print("KWH is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_POWER_K.set(map(randNumber,1,100,0,150)));

  Serial.print("Voltage is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_POWER_V.set(map(randNumber,1,100,0,150)));

  Serial.print("Current is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_POWER_C.set(map(randNumber,1,100,0,150)));  

}
#endif

#ifdef ID_S_HEATER
void heater(){
  int heater_set_point = gw.loadState(ID_S_HEATER);
  int heater_flow_state = gw.loadState(ID_S_HEATER+1);
  int heater_temp = gw.loadState(ID_S_HEATER+2);
  
  Serial.print("Heater mode is: " );
  
  if (heater_mode == 0){
    Serial.println("Off");
  }
  else if (heater_mode == 1){
    Serial.println("Heat On");
  }
  else if (heater_mode == 2){
    Serial.println("Cool On");
  }
  else if (heater_mode == 3){
    Serial.println("Auto Change Over");
  }
  else {
    Serial.print("Unknown Heater Mode: ");
    Serial.println(heater_mode);
  }
  
  gw.send(msg_S_HEATER_M.set(heater_mode));

  Serial.print("Heater is: " );
  Serial.println(heater_switch?"On":"Off");
  
  gw.send(msg_S_HEATER_S.set(heater_switch));  

  Serial.print("Heater Temperature is: " );
  Serial.println(map(randNumber,1,100,0,45));
  
  gw.send(msg_S_HEATER_T.set(map(randNumber,1,100,0,45)));
  
}
#endif

#ifdef ID_S_DISTANCE
void distance(){
  
  Serial.print("Distance is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_DISTANCE.set(map(randNumber,1,100,0,150)));

}
#endif

#ifdef ID_S_LIGHT_LEVEL
void light_level(){
  
  Serial.print("Light is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_LIGHT_LEVEL.set(map(randNumber,1,100,0,150)));

}
#endif

#ifdef ID_S_LOCK
void lock(){

  bool isLocked = gw.loadState(ID_S_LOCK);
  
  Serial.print("Lock is: " );
  
  if (isLocked) {
    Serial.println("On");
    gw.send(msg_S_LOCK.set(1));
  } else {
    Serial.println("Off");
    gw.send(msg_S_LOCK.set(0));
  }
  
}
#endif

#ifdef ID_S_IR
void ir(){

  int irVal = gw.loadState(ID_S_IR);
  
  Serial.print("Infrared is: " );
  Serial.println(irVal);
  
  gw.send(msg_S_IR_S.set(irVal));
  
}
#endif

#ifdef ID_S_WATER
void water(){
  
  Serial.print("Water flow is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_WATER_F.set(map(randNumber,1,100,0,150)));
  
  Serial.print("Water volume is: " );
  Serial.println(map(randNumber,1,100,0,150));
  
  gw.send(msg_S_WATER_V.set(map(randNumber,1,100,0,150)));

}
#endif

#ifdef ID_S_AIR_QUALITY
void air(){
  
  Serial.print("Air Quality is: " );
  Serial.println(randNumber);
  
  gw.send(msg_S_AIR_QUALITY.set(randNumber));

}
#endif

#ifdef ID_S_DUST
void dust(){
  
  Serial.print("Dust level is: " );
  Serial.println(randNumber);
  
  gw.send(msg_S_DUST.set(randNumber));

}
#endif

#ifdef ID_S_SCENE_CONTROLLER
void scene(){

  bool isSceneOn = gw.loadState(ID_S_SCENE_CONTROLLER);
  
  Serial.print("Scene is: " );
  Serial.println(isSceneOn ? "On":"Off");
  
  if(isSceneOn)
    gw.send(msg_S_SCENE_CONTROLLER_ON.set(isSceneOn));
  else
    gw.send(msg_S_SCENE_CONTROLLER_OF.set(isSceneOn));
  
}
#endif

#ifdef ID_S_CUSTOM
void custom(){
  
  Serial.print("Custom value is: " );
  Serial.println(randNumber);
  
  gw.send(msg_S_CUSTOM_2.set(randNumber));
  gw.send(msg_S_CUSTOM_3.set(randNumber));
  gw.send(msg_S_CUSTOM_4.set(randNumber));
  gw.send(msg_S_CUSTOM_5.set(randNumber));

}
#endif


void incomingMessage(const MyMessage &message) {
  switch (message.type) {
    #ifdef ID_S_LIGHT
    case V_LIGHT:
          gw.saveState(ID_S_LIGHT, message.getBool());
          Serial.print("Incoming change for ID_S_LIGHT:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println(message.getBool());
    break;
    #endif
    
    #ifdef ID_S_DIMMER
    case V_DIMMER:
          if ((message.getInt()<0)||(message.getInt()>100)) {
            Serial.println( "V_DIMMER data invalid (should be 0..100)" );
            break;
          }
          gw.saveState(ID_S_DIMMER, message.getInt());
          Serial.print("Incoming change for ID_S_DIMMER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println(message.getInt());
    break;
    #endif
    
    #ifdef ID_S_COVER    
    case V_UP:
          gw.saveState(ID_S_COVER, 0);
          Serial.print("Incoming change for ID_S_COVER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println("V_UP");
    break;
    
    case V_DOWN:
          gw.saveState(ID_S_COVER, 1);
          Serial.print("Incoming change for ID_S_COVER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println("V_DOWN");
    break;
    
    case V_STOP:
          gw.saveState(ID_S_COVER, 2);
          Serial.print("Incoming change for ID_S_COVER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println("V_STOP");
    break;
    #endif
    
    #ifdef ID_S_HEATER
    case V_HEATER:
          gw.saveState(ID_S_HEATER, message.getInt());
          Serial.print("Incoming change for ID_S_HEATER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          
          if (message.getInt() == 0){
            Serial.println("Off");
          }
          else if (message.getInt() == 1){
            Serial.println("Heat On");
          }
          else if (message.getInt() == 2){
            Serial.println("Cool On");
          }
          else if (message.getInt() == 3){
            Serial.println("Auto Change Over");
          }
          else {
            Serial.print("Unknown Heater Mode: ");
            Serial.println(message.getInt());
          }
    break;
    
    case V_HEATER_SW:
          gw.saveState(ID_S_HEATER+1, message.getBool());
          Serial.print("Incoming change for ID_S_HEATER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println((message.getBool() ? "On":"Off"));
    break;
    #endif
    
    #ifdef ID_S_LOCK
    case V_LOCK_STATUS:
          gw.saveState(ID_S_LOCK, message.getBool());
          Serial.print("Incoming change for ID_S_LOCK:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println(message.getBool()?"Locked":"Unlocked");
    break;
    #endif
    
    #ifdef ID_S_IR
    case V_IR_RECEIVE:
          gw.saveState(ID_S_IR, message.getInt());
          Serial.print("Incoming change for ID_S_IR:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println(message.getInt());
    break;
    #endif
    
    #ifdef ID_S_SCENE_CONTROLLER
    case V_SCENE_ON:
          gw.saveState(ID_S_SCENE_CONTROLLER, 1);
          Serial.print("Incoming change for ID_S_SCENE_CONTROLLER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println("On");
    break;
    case V_SCENE_OFF:
          gw.saveState(ID_S_SCENE_CONTROLLER, 0);
          Serial.print("Incoming change for ID_S_SCENE_CONTROLLER:");
          Serial.print(message.sensor);
          Serial.print(", New status: ");
          Serial.println("Off");
    break;
    #endif

    default: 
      Serial.print("Unknown/UnImplemented message type:");
      Serial.println(message.type);
  }

}







