#include <spi.h>
#include <util/delay.h>

#define BUTTON_PIN 2
#define LED_PIN 3
#define FLASH_LED_COMMAND 0x01

#define OTHER_SELECT_PIN 7
#define SELECT_OTHER digitalWrite(OTHER_SELECT_PIN, LOW);
#define DESELECT_OTHER digitalWrite(OTHER_SELECT_PIN, HIGH);

#define BUFSIZE 20
volatile unsigned char incoming[BUFSIZE];
volatile short int received=0;

// flash led that's connected to pin PD7
void flash_led(int count)
{
  pinMode(LED_PIN, OUTPUT);
  for (int i=0; i<count*2; i++) {
    digitalWrite(LED_PIN, i%2==0);
    _delay_ms(75);
  }
}

// send a SPI message to the other device - 3 bytes then go back into 
// slave mode
void send_message()
{
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK8);
  if (SPCR & (1<<MSTR)) { // if we are still in master mode
    SELECT_OTHER; // tell other device to flash LED twice
    send_spi(FLASH_LED_COMMAND); send_spi(0x02); send_spi(0x00);
    DESELECT_OTHER;
  }
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_INTERRUPT, SPI_SLAVE);
}


// called when button pushed and pin INT0 goes from 1 to 0
void button_pressed()
{
  send_message();
  _delay_ms(500);  // 'debounce'
}

// parse the data received from the other device
// currently just knows about the FLASH_LED_COMMAND
void parse_message()
{
  switch(incoming[0]) {
  case FLASH_LED_COMMAND:
    flash_led(incoming[1]);
    break;
  default:
    flash_led(20);
  }
}

// called by the SPI system when there is data ready.
// Just store the incoming data in a buffer, when we receive a
// terminating byte (0x00) call parse_message to process the data received
ISR(SPI_STC_vect)
{
  incoming[received++] = received_from_spi(0x00);
  if (received == BUFSIZE || incoming[received-1] == 0x00) {
      parse_message();
      received = 0;
   }
}

void setup()
{
  // make sure other device is unselected (pin is HIGH) and setup spi
  DESELECT_OTHER;
  pinMode(OTHER_SELECT_PIN, OUTPUT);
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_INTERRUPT, SPI_SLAVE);

  // raise interrupt when INT0 pin falls (arduino pin 2))
  // the function button_pressed will be called
  attachInterrupt(0, button_pressed, FALLING);

  // flash LED at start to indicate were ready
  flash_led(1);
}


void loop()
{
  // do nothing
}
