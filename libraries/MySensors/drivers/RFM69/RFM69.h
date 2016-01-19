// **********************************************************************************
// Driver definition for HopeRF RFM69W/RFM69HW/RFM69CW/RFM69HCW, Semtech SX1231/1231H
// **********************************************************************************
// Copyright Felix Rusu (2014), felix@lowpowerlab.com
// http://lowpowerlab.com/
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 2 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE.  See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program; if not, write 
// to the Free Software Foundation, Inc.,                
// 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//                                                        
// Licence can be viewed at                               
// http://www.fsf.org/licenses/gpl.txt                    
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#ifndef RFM69_h
#define RFM69_h
#include <Arduino.h>            //assumes Arduino IDE v1.0 or greater

#define RF69_MAX_DATA_LEN         61 // to take advantage of the built in AES/CRC we want to limit the frame size to the internal FIFO size (66 bytes - 3 bytes overhead)
#define RF69_SPI_CS               SS // SS is the SPI slave select pin, for instance D10 on atmega328

// INT0 on AVRs should be connected to RFM69's DIO0 (ex on ATmega328 it's D2, on ATmega644/1284 it's D2)
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
  #define RF69_IRQ_PIN          2
  #define RF69_IRQ_NUM          0
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega2560__)
  #define RF69_IRQ_PIN          2
  #define RF69_IRQ_NUM          2
#elif defined(__AVR_ATmega32U4__)
  #define RF69_IRQ_PIN          3
  #define RF69_IRQ_NUM          0
#else
  // ESP8266 should fall into this category now
  // TODO !! Untested code! Entering unknown territory...
  #define RF69_IRQ_PIN          2
  #define RF69_IRQ_NUM          0
#endif


#define CSMA_LIMIT          -90 // upper RX signal sensitivity threshold in dBm for carrier sense access
#define RF69_MODE_SLEEP       0 // XTAL OFF
#define	RF69_MODE_STANDBY     1 // XTAL ON
#define RF69_MODE_SYNTH	      2 // PLL ON
#define RF69_MODE_RX          3 // RX MODE
#define RF69_MODE_TX		      4 // TX MODE

//available frequency bands
#define RF69_315MHZ     31  // non trivial values to avoid misconfiguration
#define RF69_433MHZ     43
#define RF69_868MHZ     86
#define RF69_915MHZ     91

#define null                  0
#define COURSE_TEMP_COEF    -90 // puts the temperature reading in the ballpark, user can fine tune the returned value
#define RF69_BROADCAST_ADDR 255
#define RF69_CSMA_LIMIT_MS 1000

/** RFM69 class */
class RFM69 {
  public:
    static volatile byte DATA[RF69_MAX_DATA_LEN]; //!< recv/xmit buf, including hdr & crc bytes
    static volatile byte DATALEN; //!< DATALEN
    static volatile byte SENDERID; //!< SENDERID
    static volatile byte TARGETID; //!<  should match _address
    static volatile byte PAYLOADLEN; //!< PAYLOADLEN
    static volatile byte ACK_REQUESTED; //!< ACK_REQUESTED
    static volatile byte ACK_RECEIVED; //!< Should be polled immediately after sending a packet with ACK request
    static volatile int RSSI; //!<  most accurate RSSI during reception (closest to the reception)
    static volatile byte _mode; //!<  should be protected?
    
    RFM69(byte slaveSelectPin=RF69_SPI_CS, byte interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, byte interruptNum=RF69_IRQ_NUM) { //!< Constructor
      _slaveSelectPin = slaveSelectPin;
      _interruptPin = interruptPin;
      _interruptNum = interruptNum;
      _mode = RF69_MODE_STANDBY;
      _promiscuousMode = false;
      _powerLevel = 31;
      _isRFM69HW = isRFM69HW;
    }

    bool initialize(byte freqBand, byte ID, byte networkID=1); //!< initialize
    void setAddress(byte addr); //!< setAddress
    bool canSend(); //!< canSend
    void send(byte toAddress, const void* buffer, byte bufferSize, bool requestACK=false); //!< send
    bool sendWithRetry(byte toAddress, const void* buffer, byte bufferSize, byte retries=2, byte retryWaitTime=40); //!< sendWithRetry (40ms roundtrip req for 61byte packets)
    bool receiveDone(); //!< receiveDone
    bool ACKReceived(byte fromNodeID); //!< ACKReceived
    bool ACKRequested(); //!< ACKRequested
    void sendACK(const void* buffer = "", uint8_t bufferSize=0); //!< sendACK
    void setFrequency(uint32_t FRF); //!< setFrequency
    void encrypt(const char* key); //!< encrypt
    void setCS(byte newSPISlaveSelect); //!< setCS
    int readRSSI(bool forceTrigger=false); //!< readRSSI
    void promiscuous(bool onOff=true); //!< promiscuous
    void setHighPower(bool onOFF=true); //!< setHighPower (have to call it after initialize for RFM69HW)
    void setPowerLevel(byte level); //!< setPowerLevel (reduce/increase transmit power level)
    void sleep(); //!< sleep
    byte readTemperature(byte calFactor=0); //!< readTemperature (get CMOS temperature (8bit))
    void rcCalibration(); //!< rcCalibration (calibrate the internal RC oscillator for use in wide temperature variations - see datasheet section [4.3.5. RC Timer Accuracy])

    // allow hacking registers by making these public
    byte readReg(byte addr); //!< readReg
    void writeReg(byte addr, byte val); //!< writeReg
    void readAllRegs(); //!< readAllRegs

  protected:
    static void isr0(); //!< isr0
    void virtual interruptHandler(); //!< interruptHandler
    void sendFrame(byte toAddress, const void* buffer, byte size, bool requestACK=false, bool sendACK=false); //!< sendFrame

    static RFM69* selfPointer; //!< selfPointer
    byte _slaveSelectPin; //!< _slaveSelectPin
    byte _interruptPin; //!< _interruptPin
    byte _interruptNum; //!< _interruptNum
    byte _address; //!< _address
    bool _promiscuousMode; //!< _promiscuousMode
    byte _powerLevel; //!< _powerLevel
    bool _isRFM69HW; //!< _isRFM69HW
    byte _SPCR; //!< _SPCR
    byte _SPSR; //!< _SPSR

    void receiveBegin(); //!< receiveBegin
    void setMode(byte mode); //!< setMode
    void setHighPowerRegs(bool onOff); //!< setHighPowerRegs
    void select(); //!< select
    void unselect(); //!< unselect
};

#endif
