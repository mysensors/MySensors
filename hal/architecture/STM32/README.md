# MySensors STM32 Architecture Support

This directory contains the Hardware Abstraction Layer (HAL) implementation for STM32 microcontrollers using the official **STM32duino Arduino core**.

## Overview

The STM32 HAL enables MySensors to run on a wide range of STM32 microcontrollers, including:

- **STM32F1** series (Cortex-M3)
- **STM32F4** series (Cortex-M4 with FPU)

Not tested / implemented:
- **STM32F0** series (Cortex-M0)
- **STM32L0/L4** series (Low-power Cortex-M0+/M4)
- **STM32G0/G4** series (Cortex-M0+/M4)
- **STM32H7** series (Cortex-M7)

## Supported Boards

Tested on:
- **STM32F103C8 Blue Pill** (72 MHz, 64KB Flash, 20KB RAM)
- **STM32F401CC Black Pill** (84 MHz, 256KB Flash, 64KB RAM)
- **STM32F411CE Black Pill** (100 MHz, 512KB Flash, 128KB RAM)

Should work on any STM32 board supported by the STM32duino core.

## Features

### Implemented ✅
- [x] Serial communication (USB CDC and Hardware UART)
- [x] SPI interface for radios (nRF24L01+, RFM69, RFM95)
- [x] EEPROM emulation using Flash memory
- [x] Watchdog support (requires explicit initialization)
- [x] System reboot
- [x] Random number generation (using internal temperature sensor)
- [x] Unique device ID (96-bit STM32 UID)
- [x] CPU voltage reading (via VREFINT)
- [x] CPU temperature reading (via internal sensor)
- [x] CPU frequency reporting
- [x] Critical section (interrupt disable/restore)
- [x] RAM routing table support
- [x] Low-power STOP mode sleep (RTC wake-up timer)
- [x] RTC-based timed wake-up (F1: 1s resolution, F4+: subsecond)
- [x] Interrupt-based wake from sleep (GPIO EXTI)

## Pin Mapping

### STM32F4 Black Pill Example

#### nRF24L01+ Radio (SPI1)
```
nRF24     STM32
-----     -----
VCC   --> 3.3V
GND   --> GND
CE    --> PB0  (configurable via MY_RF24_CE_PIN)
CSN   --> PA4  (configurable via MY_RF24_CS_PIN)
SCK   --> PA5  (SPI1_SCK)
MOSI  --> PA7  (SPI1_MOSI)
MISO  --> PA6  (SPI1_MISO)
IRQ   --> PA3  (optional, configurable via MY_RF24_IRQ_PIN)
```

#### RFM69/RFM95 Radio (SPI1)
```
RFM69     STM32
-----     -----
VCC   --> 3.3V
GND   --> GND
NSS   --> PA4  (configurable)
SCK   --> PA5  (SPI1_SCK)
MOSI  --> PA7  (SPI1_MOSI)
MISO  --> PA6  (SPI1_MISO)
DIO0  --> PA3  (IRQ pin, configurable)
RESET --> PA2  (configurable)
```

#### Serial Communication
```
USB CDC:  Serial    (default, MY_SERIALDEVICE)
UART1:    PA9/PA10  (TX/RX)
UART2:    PA2/PA3   (TX/RX)
```

#### Optional Status LEDs
```
On-board LED: PC13 (Blue Pill) or PA5 (Black Pill)
RX LED:       Configurable via MY_DEFAULT_RX_LED_PIN
TX LED:       Configurable via MY_DEFAULT_TX_LED_PIN
ERR LED:      Configurable via MY_DEFAULT_ERR_LED_PIN
```

## PlatformIO Configuration

### platformio.ini Example

```ini
[env:blackpill_f411ce]
platform = ststm32
framework = arduino
board = blackpill_f411ce

; Upload configuration
upload_protocol = stlink

; Library dependencies
lib_deps =
    mysensors/MySensors@^2.4.0

; Monitor configuration
monitor_speed = 115200

; Debug configuration
debug_tool = stlink
```

### Supported Boards

Common `board` values for platformio.ini:
- `blackpill_f401cc` - STM32F401CC Black Pill
- `blackpill_f411ce` - STM32F411CE Black Pill
- `bluepill_f103c8` - STM32F103C8 Blue Pill
- `nucleo_f401re` - STM32F401RE Nucleo
- `nucleo_f411re` - STM32F411RE Nucleo
- `genericSTM32F103C8` - Generic F103C8
- See [PlatformIO boards](https://docs.platformio.org/en/latest/boards/index.html#st-stm32) for complete list

## Arduino IDE Configuration

1. Install STM32duino core:
   - Add board manager URL: `https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json`
   - Tools → Board → Boards Manager → Install "STM32 MCU based boards"

2. Select board:
   - Tools → Board → STM32 boards groups → Generic STM32F4 series
   - Tools → Board part number → BlackPill F411CE

3. Configure USB support:
   - Tools → USB support → CDC (generic 'Serial' supersede U(S)ART)

4. Select upload method:
   - Tools → Upload method → STM32CubeProgrammer (SWD)

## Sketch Configuration

### Basic Gateway Example

```cpp
// Enable debug
#define MY_DEBUG

// Gateway mode
#define MY_GATEWAY_SERIAL

// Radio configuration
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN PB0
#define MY_RF24_CS_PIN PA4

#include <MySensors.h>

void setup() {
    // MySensors initializes automatically
}

void presentation() {
    sendSketchInfo("STM32 Gateway", "1.0");
}

void loop() {
    // Add sensor reading code here
}
```

### Basic Sensor Node Example

```cpp
#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_RF24_CE_PIN PB0
#define MY_RF24_CS_PIN PA4
#define MY_NODE_ID 10

#include <MySensors.h>

#define CHILD_ID_TEMP 0
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void setup() {
    // Setup code
}

void presentation() {
    sendSketchInfo("STM32 Sensor", "1.0");
    present(CHILD_ID_TEMP, S_TEMP);
}

void loop() {
    float temperature = 22.5; // Read from sensor
    send(msgTemp.set(temperature, 1));
    sleep(60000); // Sleep for 1 minute
}
```

## EEPROM Emulation

The STM32 HAL uses the STM32duino EEPROM library, which provides Flash-based EEPROM emulation:

- **Size**: Configurable, typically 1-4KB
- **Location**: Last Flash page(s)
- **Wear leveling**: Implemented by STM32duino core
- **Persistence**: Survives power cycles and resets
- **Write cycles**: ~10,000 writes per page (Flash limitation)

Configuration is automatic. EEPROM size can be adjusted in the STM32duino menu or via build flags.

## Low-Power Sleep Support

### Implemented ✅

**STOP mode sleep** with RTC wake-up is fully functional:

**Supported Families:**
- **STM32F1** (Blue Pill) - RTC alarm-based, 1-second resolution
- **STM32F2/F3/F4/F7** - RTC wake-up timer, subsecond resolution
- **STM32L1/L4/L5** - RTC wake-up timer, ultra-low power
- **STM32G0/G4** - RTC wake-up timer
- **STM32H7** - RTC wake-up timer

**Power Consumption:**
- **STM32F1**: 5-20 µA (STOP mode)
- **STM32F4**: 10-50 µA (STOP mode)

**Usage:**
```cpp
sleep(60000);  // Sleep for 60 seconds
sleep(interrupt, mode, 60000);  // Sleep with interrupt wake-up
```


## Troubleshooting

### Compilation Errors

**Error: `Hardware abstraction not defined`**
- Solution: Ensure you're using STM32duino core, not Arduino STM32 (maple)
- The platform should define `ARDUINO_ARCH_STM32`

**Error: `EEPROM.h not found`**
- Solution: Update STM32duino core to latest version (2.0.0+)

### Upload Issues

**Upload fails with ST-Link**
- Check ST-Link connections (SWDIO, SWCLK, GND, 3.3V)
- Verify ST-Link firmware is up to date
- Try: `st-flash reset` to reset the chip

**DFU mode not detected**
- Set BOOT0 jumper to 1 (3.3V)
- Press reset button
- Verify with: `dfu-util -l`
- After upload, set BOOT0 back to 0 (GND)

## References

- [STM32duino Core](https://github.com/stm32duino/Arduino_Core_STM32)
- [STM32duino Wiki](https://github.com/stm32duino/Arduino_Core_STM32/wiki)
- [PlatformIO STM32 Platform](https://docs.platformio.org/en/latest/platforms/ststm32.html)
- [STM32 Reference Manuals](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)
