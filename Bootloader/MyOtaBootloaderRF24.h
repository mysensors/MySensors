#ifndef MyOtaBootloaderRF24_H
#define MyOtaBootloaderRF24_H

#include "MyOtaBootloaderHW.h"
#include "../libraries/MySensors/utility/nRF24L01.h"

#include <stdlib.h>

// hardware-agnostic interface to transceiver
// ******************************************
// static void begin()
// static void address(uint8_t addr)
// static boolean write(uint8_t next, uint8_t* packet, uint8_t length, boolean multicast)
// static bool available(uint8_t* pipe_num)
// static void read(uint8_t* buf, uint8_t pipe)

typedef enum { RF24_PA_MIN = 0,RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX, RF24_PA_ERROR } rf24_pa_dbm_e ;
typedef enum { RF24_1MBPS = 0, RF24_2MBPS, RF24_250KBPS } rf24_datarate_e;

uint64_t pipe0_reading_address; /**< Last address set on pipe 0 for reading. */

static uint8_t read_register(uint8_t reg)
{
	csnlow();
	SPItransfer( R_REGISTER | ( REGISTER_MASK & reg ) );
	uint8_t result = SPItransfer(0xff);
	csnhigh();
	return result;
}

static uint8_t write_registers(uint8_t reg, const uint8_t* buf, uint8_t len)
{
	uint8_t status;
	csnlow();
	status = SPItransfer( W_REGISTER | ( REGISTER_MASK & reg ) );
	while ( len-- )
		SPItransfer(*buf++);
	csnhigh();
	return status;
}

static uint8_t write_register(uint8_t reg, uint8_t value)
{
	uint8_t status;
	csnlow();
	status = SPItransfer( W_REGISTER | ( REGISTER_MASK & reg ) );
	SPItransfer(value);
	csnhigh();
	return status;
}

static uint8_t read_payload(uint8_t* buf, uint8_t data_len)
{
#ifdef UART_DEBUG
	uart_puts("-> ");
#endif
	uint8_t status;
	uint8_t* current = buf;
	csnlow();
	status = SPItransfer( R_RX_PAYLOAD );
	while ( data_len-- ) {
		*current = SPItransfer(0xFF);
#ifdef UART_DEBUG
		uart_puti(*current);
		uart_putc(',');
#endif
		current++;
	}
	csnhigh();
#ifdef UART_DEBUG
	uart_putc('\n');
#endif
	return status;
}

#define get_status() spiTrans(NOP)

static void powerUp(void)
{
	if (!(read_register(CONFIG) & _BV(PWR_UP))){
		write_register(CONFIG,read_register(CONFIG) | _BV(PWR_UP));
		delaym(5);
	}
}

#define convert_64_bit_to_byte_array(value, data) ((*(uint64_t *)(data)) = (uint64_t)(value))

static void startListening(void)
{
	powerUp();
	write_register(CONFIG, read_register(CONFIG) | _BV(PRIM_RX));
	write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
	if (pipe0_reading_address){
		uint8_t addressa[8];
		convert_64_bit_to_byte_array(pipe0_reading_address, addressa);
		write_registers(RX_ADDR_P0, addressa, 5);
	}
	spiTrans(FLUSH_RX);
	spiTrans(FLUSH_TX);
	cehigh();
}

static void stopListening(void)
{
	celow();
	spiTrans(FLUSH_TX);
	spiTrans(FLUSH_RX);
	write_register(CONFIG, read_register(CONFIG) & ~_BV(PRIM_RX));
}

static void write_payload(const uint8_t* buf, uint8_t data_len, const uint8_t writeType)
{
#ifdef UART_DEBUG
	uart_puts("<- ");
#endif
	const uint8_t* current = buf;
	csnlow();
	SPItransfer(writeType);
	while ( data_len-- ) {
#ifdef UART_DEBUG
		uart_puti(*current);
		uart_putc(',');
#endif
		SPItransfer(*current++);
	}
	csnhigh();
#ifdef UART_DEBUG
	uart_putc('\n');
#endif
}

static bool writem( const uint8_t* buf, uint8_t len, const bool multicast )
{
	write_payload( buf, len, multicast ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD) ;
	cehigh();
	celow();
	while( ! ( get_status()  & ( _BV(TX_DS) | _BV(MAX_RT) ))) { }
	uint8_t status = write_register(STATUS,_BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT) );
	if( status & _BV(MAX_RT)){
		spiTrans(FLUSH_TX);
		return 0;
	}
	return 1;
}

static uint8_t getDynamicPayloadSize(void)
{
	uint8_t result = 0;
	csnlow();
	SPItransfer( R_RX_PL_WID );
	result = SPItransfer(0xff);
	csnhigh();
	if(result > 32) { 
		spiTrans(FLUSH_RX); 
		return 0; 
	}
	return result;
}

static bool available(uint8_t* pipe_num)
{
	if (!(read_register(FIFO_STATUS) & _BV(RX_EMPTY) )){
		if (pipe_num) {
			uint8_t status = get_status();
			*pipe_num = ( status >> RX_P_NO ) & 0b111;
		}
		return 1;
	}
	return 0;
}

static void writeAckPayload(uint8_t pipe, const uint8_t* buf, uint8_t len)
{
	const uint8_t* current = buf;
	uint8_t data_len = len;
	csnlow();
	SPItransfer(W_ACK_PAYLOAD | ( pipe & 0b111 ) );
	while ( data_len-- )
		SPItransfer(*current++);
	csnhigh();
}

static void read( uint8_t* buf, uint8_t pipe ){
	read_payload(buf, getDynamicPayloadSize());
	write_register(STATUS,_BV(RX_DR) | _BV(MAX_RT) | _BV(TX_DS));
	writeAckPayload(pipe,&pipe, 1 );
}

static void openWritingPipe(uint64_t value)
{
	uint8_t valuea[8];
	convert_64_bit_to_byte_array(value, valuea);
	write_registers(RX_ADDR_P0, valuea, 5);
	write_registers(TX_ADDR, valuea, 5);
}

static void openReadingPipe(uint8_t child, uint64_t address)
{
	if (child == 0)
		pipe0_reading_address = address;
	uint8_t addressa[8];
	convert_64_bit_to_byte_array(address, addressa);
	if ( child < 2 )
		write_registers(RX_ADDR_P0 + child, addressa, 5);
	write_register(EN_RXADDR,read_register(EN_RXADDR) | _BV(ERX_P0 + child));
}

static boolean write(uint8_t next, uint8_t* packet, uint8_t length, boolean multicast) {
	powerUp();
	stopListening();
	openWritingPipe(TO_ADDR(next));
	bool ok = writem(packet, length, multicast);
	startListening();
	return ok;
}

static void address(uint8_t addr)
{
	if (addr != BROADCAST_ADDRESS) {
		openReadingPipe(WRITE_PIPE, TO_ADDR(addr));
		openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(addr));
	}
}

static void begin(void)
{
	pipe0_reading_address = 0;

	SPIinit();
	delaym( 10 ) ;

	write_register(SETUP_RETR, 5 << ARD | 15 << ARC);
	write_register(EN_AA, 0b00111111 & ~_BV(BROADCAST_PIPE)) ;
	write_register(RF_CH, RF24_CHANNEL);
	write_register(RF_SETUP, (read_register(RF_SETUP) & 0b11010000) | ((RF24_PA_LEVEL << 1) + 1) | ((RF24_DATARATE & 0b00000010 ) << 4) | ((RF24_DATARATE & 0b00000001 ) << 3));
	write_register(CONFIG, read_register(CONFIG) | _BV(CRCO) | _BV(EN_CRC)) ;
	write_register(STATUS, _BV(RX_DR) | _BV(TX_DS) | _BV(MAX_RT));

	csnlow();
	SPItransfer(ACTIVATE);
	SPItransfer(0x73);
	csnhigh();
	write_register(FEATURE, read_register(FEATURE) | _BV(EN_ACK_PAY) | _BV(EN_DPL) );
	write_register(DYNPD, read_register(DYNPD) | _BV(DPL_P5) | _BV(DPL_P4) | _BV(DPL_P3) | _BV(DPL_P2) | _BV(DPL_P1) | _BV(DPL_P0));

	spiTrans(FLUSH_RX);
	spiTrans(FLUSH_TX);

	powerUp(); //Power up by default when begin() is called
	write_register(CONFIG, read_register(CONFIG) & ~_BV(PRIM_RX));
	openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
}
#endif // MyOtaBootloaderRF24_H
