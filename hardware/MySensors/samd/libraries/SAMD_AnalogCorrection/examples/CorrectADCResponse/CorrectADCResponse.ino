/*
  This sketch easily and quickly finds the right ADC correction values for a particular Arduino ZERO board.
  The correction values that are found are only valid for the board where the sketch is executed.

  This example code is in the public domain.

  Written 6 May 2015 by Claudio Indellicati
*/

/*
  How to use this sketch
  
  1) Remove any connection cable, shield or jumper from your Arduino ZERO
  2) Connect pin A1 to the nearest GND pin using the shortest jumper possible
  3) Connect pin A2 to the 3.3V pin using the shortest jumper possible
  4) Connect the Arduino ZERO to your PC using a USB cable plugged in the USB programming port of the board
  5) Upload this sketch and leave the board powered on for at least one minute
  6) Open the Serial Monitor and press the reset button on the Arduino ZERO
  7) At the and of the procedure you can find logged
       - the offset and gain values for the board where the sketch has been just executed
       - the instruction line to copy/paste in the final sketch
*/

#include "SAMD_AnalogCorrection.h"

#define ADC_GND_PIN          A1
#define ADC_3V3_PIN          A2

#define ADC_READS_SHIFT      8
#define ADC_READS_COUNT      (1 << ADC_READS_SHIFT)

#define ADC_MIN_GAIN         0x0400
#define ADC_UNITY_GAIN       0x0800
#define ADC_MAX_GAIN         (0x1000 - 1)
#define ADC_RESOLUTION_BITS  12
#define ADC_RANGE            (1 << ADC_RESOLUTION_BITS)
#define ADC_TOP_VALUE        (ADC_RANGE - 1)

#define MAX_TOP_VALUE_READS  10

void setup()
{
  Serial.begin(9600);

  Serial.println("\r\nCalibrating ADC with factory values");

  analogReadResolution(ADC_RESOLUTION_BITS);

  Serial.println("\r\nReading GND and 3.3V ADC levels");
  Serial.print("   ");
  readGndLevel();
  Serial.print("   ");
  read3V3Level();

  int offsetCorrectionValue = 0;
  uint16_t gainCorrectionValue = ADC_UNITY_GAIN;
  
  Serial.print("\r\nOffset correction (@gain = ");
  Serial.print(gainCorrectionValue);
  Serial.println(" (unity gain))");

  // Set default correction values and enable correction
  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);

  for (int offset = 0; offset < (int)(ADC_OFFSETCORR_MASK >> 1); ++offset)
  {
    analogReadCorrection(offset, gainCorrectionValue);
    
    Serial.print("   Offset = ");
    Serial.print(offset);
    Serial.print(", ");

    if (readGndLevel() == 0)
    {
      offsetCorrectionValue = offset;
      break;
    }
  }

  Serial.println("\r\nGain correction");

  uint8_t topValueReadsCount = 0U;
  
  uint16_t minGain = 0U,
           maxGain = 0U;

  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);
  Serial.print("   Gain = ");
  Serial.print(gainCorrectionValue);
  Serial.print(", ");
  uint16_t highLevelRead = read3V3Level();
  
  if (highLevelRead < ADC_TOP_VALUE)
  {
    for (uint16_t gain = ADC_UNITY_GAIN + 1; gain <= ADC_MAX_GAIN; ++gain)
    {
      analogReadCorrection(offsetCorrectionValue, gain);

      Serial.print("   Gain = ");
      Serial.print(gain);
      Serial.print(", ");
      highLevelRead = read3V3Level();
      
      if (highLevelRead == ADC_TOP_VALUE)
      {
        if (minGain == 0U)
          minGain = gain;

        if (++topValueReadsCount >= MAX_TOP_VALUE_READS)
        {
          maxGain = minGain;
          break;
        }
        
        maxGain = gain;
      }

      if (highLevelRead > ADC_TOP_VALUE)
        break;
    }
  }
  else if (highLevelRead >= ADC_TOP_VALUE)
  {
    if (highLevelRead == ADC_TOP_VALUE)
      maxGain = ADC_UNITY_GAIN;

    for (uint16_t gain = ADC_UNITY_GAIN - 1; gain >= ADC_MIN_GAIN; --gain)
    {
      analogReadCorrection(offsetCorrectionValue, gain);

      Serial.print("   Gain = ");
      Serial.print(gain);
      Serial.print(", ");
      highLevelRead = read3V3Level();
      
      if (highLevelRead == ADC_TOP_VALUE)
      {
        if (maxGain == 0U)
          maxGain = gain;
        
        minGain = gain;
      }

      if (highLevelRead < ADC_TOP_VALUE)
        break;
    }
  }

  gainCorrectionValue = (minGain + maxGain) >> 1;

  analogReadCorrection(offsetCorrectionValue, gainCorrectionValue);

  Serial.println("\r\nReadings after corrections");
  Serial.print("   ");
  readGndLevel();
  Serial.print("   ");
  read3V3Level();

  Serial.println("\r\n==================");
  Serial.println("\r\nCorrection values:");
  Serial.print("   Offset = ");
  Serial.println(offsetCorrectionValue);
  Serial.print("   Gain = ");
  Serial.println(gainCorrectionValue);
  Serial.println("\r\nAdd the next line to your sketch:");
  Serial.print("   analogReadCorrection(");
  Serial.print(offsetCorrectionValue);
  Serial.print(", ");
  Serial.print(gainCorrectionValue);
  Serial.println(");");
  Serial.println("\r\n==================");
}

void loop()
{
}

uint16_t readGndLevel()
{
  uint32_t readAccumulator = 0;

  for (int i = 0; i < ADC_READS_COUNT; ++i)
    readAccumulator += analogRead(ADC_GND_PIN);

  uint16_t readValue = readAccumulator >> ADC_READS_SHIFT;
  
  Serial.print("ADC(GND) = ");
  Serial.println(readValue);

  return readValue;
}

uint16_t read3V3Level()
{
  uint32_t readAccumulator = 0;

  for (int i = 0; i < ADC_READS_COUNT; ++i)
    readAccumulator += analogRead(ADC_3V3_PIN);

  uint16_t readValue = readAccumulator >> ADC_READS_SHIFT;
  
  if (readValue < (ADC_RANGE >> 1))
    readValue += ADC_RANGE;

  Serial.print("ADC(3.3V) = ");
  Serial.println(readValue);

  return readValue;
}

