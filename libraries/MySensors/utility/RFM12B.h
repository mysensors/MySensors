// RFM12B driver definitions
// http://opensource.org/licenses/mit-license.php
// 2012-12-12 (C) felix@lowpowerlab.com
// Based on the RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)

#ifndef RFM12B_h
#define RFM12B_h

#include <inttypes.h>
#include <avr/io.h>
#include <util/crc16.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#if ARDUINO >= 100
  #include <Arduino.h> // Arduino 1.0
#else
  #include <WProgram.h> // Arduino 0022
#endif


///RF12 Driver version
#define OPTIMIZE_SPI       1  // uncomment this to write to the RFM12B @ 8 Mhz

/// RF12 CTL bit mask.
//#define RF12_HDR_CTL    0x80
/// RF12 DST bit mask.
//#define RF12_HDR_DST    0x40
/// RF12 ACK bit mask.
//#define RF12_HDR_ACK    0x20
/// RF12 HDR bit mask.
//#define RF12_HDR_MASK   0x1F
/// RF12 SENDER extracted from last packet
//#define RF12_SOURCEID   rf12_hdr & RF12_HDR_MASK

/// RF12 Maximum message size in bytes.
#define RF12_MAXDATA    128
/// Max transmit/receive buffer: 4 header + data + 2 crc bytes
#define RF_MAX          (RF12_MAXDATA + 6)

//frequency bands
#define RF12_315MHZ     0
#define RF12_433MHZ     1
#define RF12_868MHZ     2
#define RF12_915MHZ     3

//Low batteery threshold (eg 2v25 = 2.25V)
#define RF12_2v25       0
#define RF12_2v55       3
#define RF12_2v65       4
#define RF12_2v75       5
#define RF12_3v05       8
#define RF12_3v15       9
#define RF12_3v25       10

#define RF12_HDR_IDMASK      0x7F
#define RF12_HDR_ACKCTLMASK  0x80
#define RF12_DESTID   (rf12_hdr1 & RF12_HDR_IDMASK)
#define RF12_SOURCEID (rf12_hdr2 & RF12_HDR_IDMASK)

// shorthands to simplify sending out the proper ACK when requested
#define RF12_WANTS_ACK ((rf12_hdr2 & RF12_HDR_ACKCTLMASK) && !(rf12_hdr1 & RF12_HDR_ACKCTLMASK))

// options for RF12_sleep()
#define RF12_SLEEP   0
#define RF12_WAKEUP -1

/// Shorthand for RF12 group byte in rf12_buf.
#define rf12_grp        rf12_buf[0]
/// pointer to 1st header byte in rf12_buf (CTL + DESTINATIONID)
#define rf12_hdr1        rf12_buf[1]
/// pointer to 2nd header byte in rf12_buf (ACK + SOURCEID)
#define rf12_hdr2        rf12_buf[2]

/// Shorthand for RF12 length byte in rf12_buf.
#define rf12_len        rf12_buf[3]
/// Shorthand for first RF12 data byte in rf12_buf.
#define rf12_data       (rf12_buf + 4)


// pin change interrupts are currently only supported on ATmega328's
// #define PINCHG_IRQ 1    // uncomment this to use pin-change interrupts

// pins used for the RFM12B interface - yes, there *is* logic in this madness:
//  - leave RFM_IRQ set to the pin which corresponds with INT0, because the
//    current driver code will use attachInterrupt() to hook into that
//  - (new) you can now change RFM_IRQ, if you also enable PINCHG_IRQ - this
//    will switch to pin change interrupts instead of attach/detachInterrupt()
//  - use SS_DDR, SS_PORT, and SS_BIT to define the pin you will be using as
//    select pin for the RFM12B (you're free to set them to anything you like)
//  - please leave SPI_SS, SPI_MOSI, SPI_MISO, and SPI_SCK as is, i.e. pointing
//    to the hardware-supported SPI pins on the ATmega, *including* SPI_SS !
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      0
  #define SPI_SS      53    // PB0, pin 19
  #define SPI_MOSI    51    // PB2, pin 21
  #define SPI_MISO    50    // PB3, pin 22
  #define SPI_SCK     52    // PB1, pin 20
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
  #define RFM_IRQ     10
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      4
  #define SPI_SS      4
  #define SPI_MOSI    5
  #define SPI_MISO    6
  #define SPI_SCK     7
#elif defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      1
  #define SPI_SS      1     // PB1, pin 3
  #define SPI_MISO    4     // PA6, pin 7
  #define SPI_MOSI    5     // PA5, pin 8
  #define SPI_SCK     6     // PA4, pin 9
#elif defined(__AVR_ATmega32U4__) //Arduino Leonardo, MoteinoLeo
  #define RFM_IRQ     0	    // PD0, INT0, Digital3 
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  //OLD from Jeelib: #define SS_BIT      6	    // Dig10, PB6
  #define SS_BIT      0	    // Dig17, PB0
  #define SPI_SS      17    // PB0, pin 8, Digital17
  #define SPI_MISO    14    // PB3, pin 11, Digital14
  #define SPI_MOSI    16    // PB2, pin 10, Digital16
  #define SPI_SCK     15    // PB1, pin 9, Digital15
#else
  // ATmega168, ATmega328, etc.
  #define RFM_IRQ     2
  #define SS_DDR      DDRB
  #define SS_PORT     PORTB
  #define SS_BIT      2     // for PORTB: 2 = d.10, 1 = d.9, 0 = d.8
  #define SPI_SS      10    // PB2, pin 16
  #define SPI_MOSI    11    // PB3, pin 17
  #define SPI_MISO    12    // PB4, pin 18
  #define SPI_SCK     13    // PB5, pin 19
#endif 

// RF12 command codes
#define RF_RECEIVER_ON  0x82DD
#define RF_XMITTER_ON   0x823D
#define RF_IDLE_MODE    0x820D
#define RF_SLEEP_MODE   0x8205
#define RF_WAKEUP_MODE  0x8207
#define RF_TXREG_WRITE  0xB800
#define RF_RX_FIFO_READ 0xB000
#define RF_WAKEUP_TIMER 0xE000

//RF12 status bits
#define RF_LBD_BIT      0x0400
#define RF_RSSI_BIT     0x0100

// transceiver states, these determine what to do with each interrupt
enum {
  TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE,
  TXRECV,
  TXPRE1, TXPRE2, TXPRE3, TXSYN1, TXSYN2,
};

extern volatile uint8_t rf12_buf[RF_MAX];          // recv/xmit buf, including hdr & crc bytes
class RFM12B
{
  static volatile uint8_t rxfill;           // number of data bytes in rf12_buf
  static volatile int8_t rxstate;           // current transceiver state
  static volatile uint16_t rf12_crc;        // running crc value
  static uint32_t seqNum;                   // encrypted send sequence number
  static uint32_t cryptKey[4];              // encryption key to use
  static long rf12_seq;                     // seq number of encrypted packet (or -1)
  static uint8_t cs_pin;                    // chip select pin
  void (*crypter)(bool);                    // does en-/decryption (null if disabled)
  static uint8_t Byte(uint8_t out);
  static uint16_t XFERSlow(uint16_t cmd);
  static void XFER(uint16_t cmd);
  static volatile byte TARGETID; //should match _address
  
  void SPIInit();
  
	public:
    //constructor
    RFM12B():Data(rf12_data),DataLen(&rf12_buf[3]){}

    static uint8_t networkID;         // network group
    static uint8_t nodeID;            // address of this node
    static const byte DATAMAXLEN;
    volatile uint8_t* Data;
    volatile uint8_t* DataLen;
    
    static void InterruptHandler();
    
    //Defaults: Group: 0xAA=170, transmit power: 0(max), KBPS: 38.3Kbps (air transmission baud - has to be same on all radios in same group)
  	void Initialize(uint8_t nodeid, uint8_t freqBand, uint8_t groupid=0xAA, uint8_t txPower=0, uint8_t airKbps=0x08, uint8_t lowVoltageThreshold=RF12_2v75);
    void SetCS(uint8_t pin);
    void ReceiveStart();
    bool ReceiveComplete();
    bool CanSend();
    uint16_t Control(uint16_t cmd);
    
    void SendStart(uint8_t toNodeId, bool requestACK=false, bool sendACK=false);
    void SendStart(uint8_t toNodeId, const void* sendBuf, uint8_t sendLen, bool requestACK=false, bool sendACK=false, uint8_t waitMode=SLEEP_MODE_STANDBY);
    void SendACK(const void* sendBuf = "", uint8_t sendLen=0, uint8_t waitMode=SLEEP_MODE_IDLE);
    void Send(uint8_t toNodeId, const void* sendBuf, uint8_t sendLen, bool requestACK = false, uint8_t waitMode=SLEEP_MODE_STANDBY);
    void SendWait(uint8_t waitMode=0);

    void OnOff(uint8_t value);
    void Sleep(char n);
    void Sleep();
    void Wakeup();
    
    volatile uint8_t * GetData();
    uint8_t GetDataLen(); //how many bytes were received
    uint8_t GetSender();
    bool LowBattery();
    bool ACKRequested();
    bool ACKReceived(uint8_t fromNodeID=0);
    static void CryptFunction(bool sending);
    void Encrypt(const uint8_t* key, uint8_t keyLen = 16);
    bool CRCPass() { return rf12_crc == 0; }
};

#endif
