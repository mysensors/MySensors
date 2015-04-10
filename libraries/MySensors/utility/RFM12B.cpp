// RFM12B driver definitions
// http://opensource.org/licenses/mit-license.php
// 2012-12-12 (C) felix@lowpowerlab.com
// Based on the RFM12 driver from jeelabs.com (2009-02-09 <jc@wippler.nl>)

#include "RFM12B.h"

uint8_t RFM12B::cs_pin;                // CS pin for SPI
uint8_t RFM12B::nodeID;                // address of this node
uint8_t RFM12B::networkID;             // network group ID
long RFM12B::rf12_seq;
uint32_t RFM12B::seqNum;
uint32_t RFM12B::cryptKey[4];
volatile uint8_t RFM12B::rxfill;       // number of data bytes in rf12_buf
volatile int8_t RFM12B::rxstate;       // current transceiver state
volatile uint16_t RFM12B::rf12_crc;    // running crc value
volatile uint8_t rf12_buf[RF_MAX];     // recv/xmit buf, including hdr & crc bytes
volatile byte RFM12B::TARGETID;        //should match _address


// function to set chip select
void RFM12B::SetCS(uint8_t arduinoPin)
{
  if (arduinoPin==10) cs_pin = 2;
  if (arduinoPin== 9) cs_pin = 1;
  if (arduinoPin== 8) cs_pin = 0;
}

void RFM12B::SPIInit() {
  bitSet(SS_PORT, cs_pin);
  bitSet(SS_DDR, cs_pin);
  digitalWrite(SPI_SS, 1);
  pinMode(SPI_SS, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_MISO, INPUT);
  pinMode(SPI_SCK, OUTPUT);
#ifdef SPCR    
  SPCR = _BV(SPE) | _BV(MSTR);
#if F_CPU > 10000000
  // use clk/2 (2x 1/4th) for sending (and clk/8 for recv, see XFERSlow)
  SPSR |= _BV(SPI2X);
#endif
#else
  // ATtiny
  USICR = bit(USIWM0);
#endif    
  pinMode(RFM_IRQ, INPUT);
  digitalWrite(RFM_IRQ, 1); // pull-up
}

uint8_t RFM12B::Byte(uint8_t out) {
#ifdef SPDR
  SPDR = out;
  // this loop spins 4 usec with a 2 MHz SPI clock
  while (!(SPSR & _BV(SPIF)));
  return SPDR;
#else
  // ATtiny
  USIDR = out;
  byte v1 = bit(USIWM0) | bit(USITC);
  byte v2 = bit(USIWM0) | bit(USITC) | bit(USICLK);
#if F_CPU <= 5000000
  // only unroll if resulting clock stays under 2.5 MHz
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
  USICR = v1; USICR = v2;
#else
  for (uint8_t i = 0; i < 8; ++i) {
    USICR = v1;
    USICR = v2;
  }
#endif
  return USIDR;
#endif
}

uint16_t RFM12B::XFERSlow(uint16_t cmd) {
  // slow down to under 2.5 MHz
#if F_CPU > 10000000
  bitSet(SPCR, SPR0);
#endif
  bitClear(SS_PORT, cs_pin);
  uint16_t reply = Byte(cmd >> 8) << 8;
  reply |= Byte(cmd);
  bitSet(SS_PORT, cs_pin);
#if F_CPU > 10000000
  bitClear(SPCR, SPR0);
#endif
  return reply;
}

void RFM12B::XFER(uint16_t cmd) {
#if OPTIMIZE_SPI
  // writing can take place at full speed, even 8 MHz works
  bitClear(SS_PORT, cs_pin);
  Byte(cmd >> 8) << 8;
  Byte(cmd);
  bitSet(SS_PORT, cs_pin);
#else
  XFERSlow(cmd);
#endif
}

// Call this once with params:
// - node ID (0-31)
// - frequency band (RF12_433MHZ, RF12_868MHZ, RF12_915MHZ)
// - networkid [optional - default = 170] (0-255 for RF12B, only 212 allowed for RF12)
// - txPower [optional - default = 0 (max)] (7 is min value)
// - AirKbps [optional - default = 38.31Kbps]
// - lowVoltageThreshold [optional - default = RF12_2v75]
void RFM12B::Initialize(uint8_t ID, uint8_t freqBand, uint8_t networkid, uint8_t txPower, uint8_t airKbps, uint8_t lowVoltageThreshold)
{
  //while(millis()<60);
  cs_pin = SS_BIT;
  nodeID = ID;
  networkID = networkid;
  SPIInit();
  XFER(0x0000); // intitial SPI transfer added to avoid power-up problem
  XFER(RF_SLEEP_MODE); // DC (disable clk pin), enable lbd
  
  // wait until RFM12B is out of power-up reset, this takes several *seconds*
  XFER(RF_TXREG_WRITE); // in case we're still in OOK mode
  while (digitalRead(RFM_IRQ) == 0)
    XFER(0x0000);
      
  XFER(0x80C7 | (freqBand << 4)); // EL (ena TX), EF (ena RX FIFO), 12.0pF 
  XFER(0xA640); // Frequency is exactly 434/868/915MHz (whatever freqBand is)
  XFER(0xC600 + airKbps);   //Air transmission baud rate: 0x08= ~38.31Kbps
  XFER(0x94A2);             // VDI,FAST,134kHz,0dBm,-91dBm 
  XFER(0xC2AC);             // AL,!ml,DIG,DQD4 
  if (networkID != 0) {
    XFER(0xCA83);           // FIFO8,2-SYNC,!ff,DR 
    XFER(0xCE00 | networkID); // SYNC=2DXX; 
  } else {
    XFER(0xCA8B); // FIFO8,1-SYNC,!ff,DR 
    XFER(0xCE2D); // SYNC=2D; 
  }
  XFER(0xC483); // @PWR,NO RSTRIC,!st,!fi,OE,EN 
  XFER(0x9850 | (txPower > 7 ? 7 : txPower)); // !mp,90kHz,MAX OUT               //last byte=power level: 0=highest, 7=lowest
  XFER(0xCC77); // OB1,OB0, LPX,!ddy,DDIT,BW0 
  XFER(0xE000); // NOT USE
  XFER(0xC800); // NOT USE
  XFER(0xC043); // Clock output (1.66MHz), Low Voltage threshold (2.55V)

  rxstate = TXIDLE;
#if PINCHG_IRQ
  #if RFM_IRQ < 8
    if (nodeID != 0) {
      bitClear(DDRD, RFM_IRQ);      // input
      bitSet(PORTD, RFM_IRQ);       // pull-up
      bitSet(PCMSK2, RFM_IRQ);      // pin-change
      bitSet(PCICR, PCIE2);         // enable
    } else
      bitClear(PCMSK2, RFM_IRQ);
  #elif RFM_IRQ < 14
    if (nodeID != 0) {
      bitClear(DDRB, RFM_IRQ - 8);  // input
      bitSet(PORTB, RFM_IRQ - 8);   // pull-up
      bitSet(PCMSK0, RFM_IRQ - 8);  // pin-change
      bitSet(PCICR, PCIE0);         // enable
    } else
      bitClear(PCMSK0, RFM_IRQ - 8);
  #else
    if (nodeID != 0) {
      bitClear(DDRC, RFM_IRQ - 14); // input
      bitSet(PORTC, RFM_IRQ - 14);  // pull-up
      bitSet(PCMSK1, RFM_IRQ - 14); // pin-change
      bitSet(PCICR, PCIE1);         // enable
    } else
      bitClear(PCMSK1, RFM_IRQ - 14);
  #endif
#else
  if (nodeID != 0)
    attachInterrupt(0, RFM12B::InterruptHandler, LOW);
  else
    detachInterrupt(0);
#endif
}

// access to the RFM12B internal registers with interrupts disabled
uint16_t RFM12B::Control(uint16_t cmd) {
#ifdef EIMSK
  bitClear(EIMSK, INT0);
  uint16_t r = XFERSlow(cmd);
  bitSet(EIMSK, INT0);
#else
  // ATtiny
  bitClear(GIMSK, INT0);
  uint16_t r = XFERSlow(cmd);
  bitSet(GIMSK, INT0);
#endif
    return r;
}

void RFM12B::InterruptHandler() {
  // a transfer of 2x 16 bits @ 2 MHz over SPI takes 2x 8 us inside this ISR
  // correction: now takes 2 + 8 µs, since sending can be done at 8 MHz
  XFER(0x0000);
  
  if (rxstate == TXRECV) {
    uint8_t in = XFERSlow(RF_RX_FIFO_READ);

    if (rxfill == 0 && networkID != 0)
      rf12_buf[rxfill++] = networkID;

    //Serial.print(out, HEX); Serial.print(' ');
    rf12_buf[rxfill++] = in;
    rf12_crc = _crc16_update(rf12_crc, in);

    if (rxfill >= rf12_len + 6 || rxfill >= RF_MAX)
      XFER(RF_IDLE_MODE);
  } else {
    uint8_t out;

      if (rxstate < 0) {
        uint8_t pos = 4 + rf12_len + rxstate++;
        out = rf12_buf[pos];
        rf12_crc = _crc16_update(rf12_crc, out);
      } else
        switch (rxstate++) {
          case TXSYN1: out = 0x2D; break;
          case TXSYN2: out = networkID; rxstate = -(3 + rf12_len); break;
          case TXCRC1: out = rf12_crc; break;
          case TXCRC2: out = rf12_crc >> 8; break;
          case TXDONE: XFER(RF_IDLE_MODE); // fall through
          default:     out = 0xAA;
        }
        
    //Serial.print(out, HEX); Serial.print(' ');
    XFER(RF_TXREG_WRITE + out);
  }
}


#if PINCHG_IRQ
  #if RFM_IRQ < 8
    ISR(PCINT2_vect) {
      while (!bitRead(PIND, RFM_IRQ))
        RFM12B::InterruptHandler();
    }
  #elif RFM_IRQ < 14
    ISR(PCINT0_vect) {
      while (!bitRead(PINB, RFM_IRQ - 8))
        RFM12B::InterruptHandler();
    }
  #else
    ISR(PCINT1_vect) {
      while (!bitRead(PINC, RFM_IRQ - 14))
        RFM12B::InterruptHandler();
    }
  #endif
#endif

void RFM12B::ReceiveStart() {
  TARGETID = 0;

  rxfill = rf12_len = 0;
  rf12_crc = ~0;
  if (networkID != 0)
    rf12_crc = _crc16_update(~0, networkID);
  rxstate = TXRECV;
  XFER(RF_RECEIVER_ON);
}

bool RFM12B::ReceiveComplete() {
  if (rxstate == TXRECV && (rxfill >= rf12_len + 6 || rxfill >= RF_MAX)) {
    rxstate = TXIDLE;
    if (rf12_len > RF12_MAXDATA)
      rf12_crc = 1; // force bad crc if packet length is invalid
    if (RF12_DESTID == 0 || RF12_DESTID == nodeID) { //if (!(rf12_hdr & RF12_HDR_DST) || (nodeID & NODE_ID) == 31 || (rf12_hdr & RF12_HDR_MASK) == (nodeID & NODE_ID)) {
      if (rf12_crc == 0 && crypter != 0)
        crypter(false);
      else
        rf12_seq = -1;
      return true; // it's a broadcast packet or it's addressed to this node
    }
  }
  if (rxstate == TXIDLE)
    ReceiveStart();
  return false;
}

bool RFM12B::CanSend() {
  // no need to test with interrupts disabled: state TXRECV is only reached
  // outside of ISR and we don't care if rxfill jumps from 0 to 1 here
  if (rxstate == TXRECV && rxfill == 0 && (Byte(0x00) & (RF_RSSI_BIT >> 8)) == 0) {
    XFER(RF_IDLE_MODE); // stop receiver
    //XXX just in case, don't know whether these RF12 reads are needed!
    // rf12_XFER(0x0000); // status register
    // rf12_XFER(RF_RX_FIFO_READ); // fifo read
    rxstate = TXIDLE;
    return true;
  }
  return false;
}

void RFM12B::SendStart(uint8_t toNodeID, bool requestACK, bool sendACK) {
  rf12_hdr1 = toNodeID | (sendACK ? RF12_HDR_ACKCTLMASK : 0);
  rf12_hdr2 = nodeID | (requestACK ? RF12_HDR_ACKCTLMASK : 0);
  if (crypter != 0) crypter(true);
  rf12_crc = ~0;
  rf12_crc = _crc16_update(rf12_crc, networkID);
  rxstate = TXPRE1;
  XFER(RF_XMITTER_ON); // bytes will be fed via interrupts
}

void RFM12B::SendStart(uint8_t toNodeID, const void* sendBuf, uint8_t sendLen, bool requestACK, bool sendACK, uint8_t waitMode) {
  rf12_len = sendLen;
  memcpy((void*) rf12_data, sendBuf, sendLen);
  SendStart(toNodeID, requestACK, sendACK);
  SendWait(waitMode);
}

/// Should be called immediately after reception in case sender wants ACK
void RFM12B::SendACK(const void* sendBuf, uint8_t sendLen, uint8_t waitMode) {
  while (!CanSend()) ReceiveComplete();
  SendStart(RF12_SOURCEID, sendBuf, sendLen, false, true, waitMode);
}

void RFM12B::Send(uint8_t toNodeID, const void* sendBuf, uint8_t sendLen, bool requestACK, uint8_t waitMode)
{
  while (!CanSend()) ReceiveComplete();
  SendStart(toNodeID, sendBuf, sendLen, requestACK, false, waitMode);
}

void RFM12B::SendWait(uint8_t waitMode) {
  // wait for packet to actually finish sending
  // go into low power mode, as interrupts are going to come in very soon
  while (rxstate != TXIDLE)
    if (waitMode) {
      // power down mode is only possible if the fuses are set to start
      // up in 258 clock cycles, i.e. approx 4 us - else must use standby!
      // modes 2 and higher may lose a few clock timer ticks
      set_sleep_mode(waitMode == 3 ? SLEEP_MODE_PWR_DOWN :
      #ifdef SLEEP_MODE_STANDBY
                     waitMode == 2 ? SLEEP_MODE_STANDBY :
      #endif
                     SLEEP_MODE_IDLE);
      sleep_mode();
    }
}

void RFM12B::OnOff(uint8_t value) {
  XFER(value ? RF_XMITTER_ON : RF_IDLE_MODE);
}

void RFM12B::Sleep(char n) {
  if (n < 0)
    Control(RF_IDLE_MODE);
  else {
    Control(RF_WAKEUP_TIMER | 0x0500 | n);
    Control(RF_SLEEP_MODE);
    if (n > 0)
      Control(RF_WAKEUP_MODE);
  }
  rxstate = TXIDLE;
}
void RFM12B::Sleep() { Sleep(0); }
void RFM12B::Wakeup() { Sleep(-1); }

bool RFM12B::LowBattery() {
  return (Control(0x0000) & RF_LBD_BIT) != 0;
}

uint8_t RFM12B::GetSender(){
  return RF12_SOURCEID;
}

volatile uint8_t * RFM12B::GetData() { return rf12_data; }
uint8_t RFM12B::GetDataLen() { return *DataLen; }
bool RFM12B::ACKRequested() { return RF12_WANTS_ACK; }

/// Should be polled immediately after sending a packet with ACK request
bool RFM12B::ACKReceived(uint8_t fromNodeID) {
  if (ReceiveComplete())
    return CRCPass() &&
           RF12_DESTID == nodeID &&
          (RF12_SOURCEID == fromNodeID || fromNodeID == 0) &&
          (rf12_hdr1 & RF12_HDR_ACKCTLMASK) &&
          !(rf12_hdr2 & RF12_HDR_ACKCTLMASK);
  return false;
}


// XXTEA by David Wheeler, adapted from http://en.wikipedia.org/wiki/XXTEA
#define DELTA 0x9E3779B9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (cryptKey[(uint8_t)((p&3)^e)] ^ z)))
void RFM12B::CryptFunction(bool sending) {
  uint32_t y, z, sum, *v = (uint32_t*) rf12_data;
  uint8_t p, e, rounds = 6;
  
  if (sending) {
    // pad with 1..4-byte sequence number
    *(uint32_t*)(rf12_data + rf12_len) = ++seqNum;
    uint8_t pad = 3 - (rf12_len & 3);
    rf12_len += pad;
    rf12_data[rf12_len] &= 0x3F;
    rf12_data[rf12_len] |= pad << 6;
    ++rf12_len;
    // actual encoding
    char n = rf12_len / 4;
    if (n > 1) {
      sum = 0;
      z = v[n-1];
      do {
        sum += DELTA;
        e = (sum >> 2) & 3;
        for (p=0; p<n-1; p++)
            y = v[p+1], z = v[p] += MX;
        y = v[0];
        z = v[n-1] += MX;
      } while (--rounds);
    }
  } else if (rf12_crc == 0) {
    // actual decoding
    char n = rf12_len / 4;
    if (n > 1) {
      sum = rounds*DELTA;
      y = v[0];
      do {
        e = (sum >> 2) & 3;
        for (p=n-1; p>0; p--)
          z = v[p-1], y = v[p] -= MX;
        z = v[n-1];
        y = v[0] -= MX;
      } while ((sum -= DELTA) != 0);
    }
    // strip sequence number from the end again
    if (n > 0) {
      uint8_t pad = rf12_data[--rf12_len] >> 6;
      rf12_seq = rf12_data[rf12_len] & 0x3F;
      while (pad-- > 0)
        rf12_seq = (rf12_seq << 8) | rf12_data[--rf12_len];
    }
  }
}

void RFM12B::Encrypt(const uint8_t* key, uint8_t keyLen) {
  // by using a pointer to CryptFunction, we only link it in when actually used
  if (key != 0) {
    for (uint8_t i = 0; i < keyLen; ++i)
      ((uint8_t*) cryptKey)[i] = key[i];
    crypter = CryptFunction;
  } else crypter = 0;
}
