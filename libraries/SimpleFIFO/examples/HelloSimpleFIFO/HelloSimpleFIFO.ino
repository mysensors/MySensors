#include <SimpleFIFO.h>

void setup() {
  Serial.begin(9600);
  
  SimpleFIFO<int,10> sFIFO; //store 10 ints
  
  sFIFO.enqueue(1);
  sFIFO.enqueue(2);
  sFIFO.enqueue(3);
  sFIFO.enqueue(4);
  sFIFO.enqueue(5);
  
  Serial.print(F("Peek: "));
  Serial.println(sFIFO.peek());
  
  for (int i=0; i<sFIFO.count(); i++) {
    Serial.print(F("Dequeue "));
    Serial.println(sFIFO.dequeue());
  }
}

void loop() {
  //nothing
}


