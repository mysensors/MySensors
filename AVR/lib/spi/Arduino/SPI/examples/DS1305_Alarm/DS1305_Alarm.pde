#include <spi.h>
#include <ds1305.h>

#define DS1305_PIN 4
#define LED_PIN 3

// following two methods required by DS1305 library to select/deselect device
void select_ds1305(void)
{
  digitalWrite(DS1305_PIN, HIGH);
}

void deselect_ds1305(void)
{
  digitalWrite(DS1305_PIN, LOW);
}

// flash led that's connected to pin 3
void flash_led(void)
{
  pinMode(LED_PIN, OUTPUT);
  for (int i=0; i<10; i++) {
    digitalWrite(LED_PIN, i%2==0);
    delay(50);
  }
}

// called when pin INT0 goes from 1 to 0
ISR(INT0_vect)
{
  // get alarm value to clear alarm interrupt flag on DS1305
  DS1305_DATETIME alarm;
  get_alarm0(&alarm);
  flash_led();
}

void setup()
{
  // flash LED at start to indicate were about to start
  flash_led();
  flash_led();
  
  // pin used to enable DS1305
  pinMode(DS1305_PIN, OUTPUT);

  // Make sure ADC is unselected and setup spi
  deselect_ds1305();
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK16);
  set_control(0);

  // set current date/time to Thursday 06/05/09 20:32:30
  DS1305_DATETIME current = {0x30, 0x32, 0x20, 0x05, 0x06, 0x05, 0x09};
  set_time(&current);
  
  // change the value of alarm to the required time/granularity:
  // raise alarm every day (at 20:32:00):
  // DS1305_DATETIME alarm = {0x00, 0x32, 0x20, 0x00 | DS1305_ALARM_SET};
  // raise alarm every minute (when seconds = 00):
  // DS1305_DATETIME alarm = {0x00, 0x32 | DS1305_ALARM_SET,
  //		   0x20 | DS1305_ALARM_SET, 0x05 | DS1305_ALARM_SET};
  // raise alarm every second:
  DS1305_DATETIME alarm = {0x55 | DS1305_ALARM_SET, 0x32 | DS1305_ALARM_SET,
			   0x20 | DS1305_ALARM_SET, 0x05 | DS1305_ALARM_SET};
  set_alarm0(&alarm);
  // turn on timer and make alarm0 lower INT0 pin when alarm is triggered
  set_control(DS1305_EOSC | DS1305_INTCN | DS1305_AIE0);

  // raise interrupt when INT0 pin falls (pin PD0 at AT90usbXXX, pin PD2 on ATmegaXXX (arduino pin 2))
  // the code in ISR(INT0_vect) above will be called
  EICRA = (1<<ISC01) | (0<<ISC00);
  // enable interrupts
  EIMSK = (1<<INT0);
  sei();
}

void loop()
{
  // do nothing - all action happens in interrupt
}
