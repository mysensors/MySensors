#ifndef MYSBootloaderRF24_H
#define MYSBootloaderRF24_H

#include "MYSBootloaderHW.h"
#include "nRF24L01.h"

typedef enum { RF24_PA_MIN = 0,RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX, RF24_PA_ERROR } rf24_pa_dbm_e ;
typedef enum { RF24_1MBPS = 0, RF24_2MBPS, RF24_250KBPS } rf24_datarate_e;

#define _write_register(reg) ( (reg) | W_REGISTER)
#define _read_register(reg) ( (reg) | R_REGISTER)
#define addr_width 5

uint64_t pipe0_reading_address = 0;

static uint8_t BurstReadAddress(uint8_t addr, uint8_t* buf, uint8_t len) {
	csnlow();
	uint8_t status = SPItransfer( addr );
	while ( len-- ) {
		status = SPItransfer(NOP);
		*buf++ = status;
	}
	csnhigh();
	return status;
}

static uint8_t BurstWriteAddress(uint8_t addr, uint8_t* buf, uint8_t len) {
	csnlow();
	uint8_t status = SPItransfer( addr );
	while ( len-- ) {
		status = SPItransfer(*buf++);
	}
	csnhigh();
	return status;
}

static uint8_t ReadAddress(uint8_t addr) {
	return BurstReadAddress(addr,NULL,1);
}


static uint8_t WriteAddress(uint8_t addr, uint8_t val) {
	return BurstWriteAddress(addr,&val,1);
}

#define BurstWriteRegister(reg,buf,len) BurstWriteAddress(_write_register(reg), buf, len)
#define WriteRegister(reg,val) WriteAddress(_write_register(reg), val)
#define BurstReadRegister(reg,buf,len) BurstReadAddress(_read_register(reg), buf, len)
#define ReadRegister(reg) ReadAddress(_read_register(reg))
#define get_status() BurstReadAddress(NOP, NULL, 0)

void Flush_RX(void) {
	BurstWriteAddress(FLUSH_RX, NULL, 0);
}

void Flush_TX(void) {
	BurstWriteAddress(FLUSH_TX, NULL, 0);
}

static void Flush_RXTX_CLI(void) {
	// flush RX and TX buffer
	Flush_RX();
	Flush_TX();
	// clear registers
	WriteRegister(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
}

static void startListening(void) {
	// for size reasons, we do not read back the register content, set PRIM_RX
	WriteRegister(CONFIG, _BV(PWR_UP) | _BV(CRCO) | _BV(EN_CRC) | _BV(PRIM_RX)) ;
	Flush_RXTX_CLI();
	// write pipe0 address
	if (pipe0_reading_address){
		BurstWriteRegister(RX_ADDR_P0, (uint8_t*)&pipe0_reading_address, addr_width);
	}
	cehigh();
}

static void stopListening(void) {
	celow();
	Flush_RXTX_CLI();
	// for size reasons, we do not read back the register content, clear PRIM_RX
	WriteRegister(CONFIG, _BV(PWR_UP) | _BV(CRCO) | _BV(EN_CRC) | ~_BV(PRIM_RX)) ;
}



static boolean write_buf(uint8_t* buf, uint8_t len, const bool multicast ) {
	// write payload to FIFO
	BurstWriteRegister( multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD, buf, len) ;
	// CE pulse to start transmission
	cehigh();
	// IMPORTANT: minimum CE pulse width 10us, see nRF24L01 specs
	_delay_us(CE_PULSE_LENGTH);
	// start transmitting
	celow();
	

	// wait until sent or ACKed, here we potentially have a deadlock when transmitter not connected/not working => wdt recovers
	while( ! ( get_status()  & ( _BV(TX_DS) | _BV(MAX_RT) ))) { }
	// read interrupts and clear
	uint8_t status = WriteRegister(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
	// max retries? clear FIFO
	if( status & _BV(MAX_RT)){
		Flush_TX();
		return false;
	}
	return true;
}

static uint8_t getDynamicPayloadSize(void) {
	uint8_t result = ReadAddress(R_RX_PL_WID);
	if(result > 32) {
		Flush_RX();
		result = 0;
	}
	return result;
}


static boolean available(uint8_t* pipe_num) {
	if (!(ReadAddress(FIFO_STATUS) & _BV(RX_EMPTY) )){
		if (pipe_num) {
			uint8_t status = get_status();
			*pipe_num = ( status >> RX_P_NO ) & 0b111;
		}
		return true;
	}
	return false;
}


static uint8_t readMessage(uint8_t* buf) {
	// read payload
	uint8_t PL_LEN = getDynamicPayloadSize();
	BurstReadRegister(R_RX_PAYLOAD,buf,PL_LEN);
	// reset interrupts
	WriteRegister(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
	return PL_LEN;
}

static void openWritingPipe(uint64_t value) {
	BurstWriteRegister(RX_ADDR_P0, (uint8_t*)&value, addr_width);
	BurstWriteRegister(TX_ADDR, (uint8_t*)&value, addr_width);
}

static void openReadingPipe(uint8_t pipe, uint64_t address) {
	// If this is pipe 0, cache the address.  This is needed because openWritingPipe() will overwrite the pipe 0 address, so
	// startListening() will have to restore it
	if (pipe == WRITE_PIPE) {
		memcpy(&pipe0_reading_address,&address,addr_width);	
	}
	// only write full address for pipe 0 and 1
	// since we do not use the BROADCASTING PIPE in this bootloader, we can remove the check
	//if ( pipe < 2 ) 
	{
		BurstWriteRegister(RX_ADDR_P0 + pipe, (uint8_t*)&address, addr_width);
	}
	
	
}

static boolean write(uint8_t destination, uint8_t* buf, uint8_t len, boolean multicast) {
	stopListening();
	openWritingPipe(TO_ADDR(destination));
	boolean result = write_buf(buf, len, multicast);
	startListening();
	return result;
}

static void setAddress(uint8_t addr)
{
	if (addr != BROADCAST_ADDRESS) {
		openReadingPipe(WRITE_PIPE, TO_ADDR(addr));
		openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(addr));
		// enable pipe
		WriteRegister(EN_RXADDR,_BV(ERX_P0 + WRITE_PIPE) | _BV(ERX_P0 + CURRENT_NODE_PIPE));
	}
}



static void RFinit(void){
	// set address width
	WriteRegister(SETUP_AW, (addr_width-2) % 4);
	// auto retransmit delay 1500us, auto retransmit count 15
	WriteRegister(SETUP_RETR, 5 << ARD | 15 << ARC);
	// enable auto ack on all pipes, except broadcasting pipe
	WriteRegister(EN_AA, 0b00111111 & ~_BV(BROADCAST_PIPE)) ;
	// set channel
	WriteRegister(RF_CH, RF24_CHANNEL);
	// set data rate
	WriteRegister(RF_SETUP, ((RF24_DATARATE & 0b00000010 ) << 4) | ((RF24_DATARATE & 0b00000001 ) << 3) | ((RF24_PA_LEVEL << 1) + 1));
	// flush RX and TX FIFO, clear interrupts
	Flush_RXTX_CLI();
	// activate to unlock features
	WriteAddress(ACTIVATE,0x73);
	// enable payload with ACK and dynamic payload length
	WriteRegister(FEATURE, _BV(EN_ACK_PAY) | _BV(EN_DPL) );
	// Enable dynamic payload length on all pipes
	WriteRegister(DYNPD, _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));
	//powerUp, enable 16bit CRC, no RX mode
	WriteRegister(CONFIG, _BV(PWR_UP) | _BV(CRCO) | _BV(EN_CRC) | ~_BV(PRIM_RX));
	// stabilize 5ms
	_delay_ms(5);
}

#endif // MYSBootloaderRF24_H
