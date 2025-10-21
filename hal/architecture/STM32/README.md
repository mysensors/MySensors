# MySensors STM32 Architecture Support

This directory contains the Hardware Abstraction Layer (HAL) implementation for STM32 microcontrollers using the official **STM32duino Arduino core**.

## Overview

The STM32 HAL enables MySensors to run on a wide range of STM32 microcontrollers, including:

- **STM32F0** series (Cortex-M0)
- **STM32F1** series (Cortex-M3) - Note: This is separate from the old STM32F1 maple implementation
- **STM32F4** series (Cortex-M4 with FPU)
- **STM32L0/L4** series (Low-power Cortex-M0+/M4)
- **STM32G0/G4** series (Cortex-M0+/M4)
- **STM32H7** series (Cortex-M7)

## Supported Boards

Tested on:
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

### Planned 🔄
- [ ] Low-power sleep modes (STOP, STANDBY)
- [ ] RTC-based timekeeping
- [ ] Interrupt-based wake from sleep
- [ ] Free memory reporting (heap analysis)

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

; Build flags
build_flags =
    -D MY_DEBUG
    -D MY_BAUD_RATE=115200
    -D MY_GATEWAY_SERIAL
    -D MY_RADIO_RF24
    -D MY_RF24_CE_PIN=PB0
    -D MY_RF24_CS_PIN=PA4
    -D MY_RF24_PA_LEVEL=RF24_PA_LOW

; Library dependencies
lib_deps =
    mysensors/MySensors@^2.4.0
    ; Add radio-specific libraries if needed

; Monitor configuration
monitor_speed = 115200

; Debug configuration
debug_tool = stlink
```

### Supported Boards

Common `board` values for platformio.ini:
- `blackpill_f401cc` - STM32F401CC Black Pill
- `blackpill_f411ce` - STM32F411CE Black Pill (recommended)
- `bluepill_f103c8` - STM32F103C8 Blue Pill (use old STM32F1 HAL instead)
- `nucleo_f401re` - STM32F401RE Nucleo
- `nucleo_f411re` - STM32F411RE Nucleo
- `genericSTM32F103C8` - Generic F103C8
- See [PlatformIO boards](https://docs.platformio.org/en/latest/boards/index.html#st-stm32) for complete list

### Upload Methods

Supported `upload_protocol` options:
- `stlink` - ST-Link V2 programmer (recommended)
- `dfu` - USB DFU bootloader (requires boot0 jumper)
- `serial` - Serial bootloader (requires FTDI adapter)
- `jlink` - Segger J-Link
- `blackmagic` - Black Magic Probe

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

## Low-Power Considerations

### Current Status
Sleep modes are **NOT YET IMPLEMENTED** in this initial release. Calling `sleep()` functions will return `MY_SLEEP_NOT_POSSIBLE`.

### Future Implementation
The STM32 supports several low-power modes:
- **Sleep mode**: ~10mA (CPU stopped, peripherals running)
- **Stop mode**: ~10-100µA (CPU and most peripherals stopped)
- **Standby mode**: ~1-10µA (only backup domain active)

Implementation will use:
- RTC for timed wake-up
- EXTI for interrupt wake-up
- Backup SRAM for state retention

## Troubleshooting

### Compilation Errors

**Error: `Hardware abstraction not defined`**
- Solution: Ensure you're using STM32duino core, not Arduino STM32 (maple)
- The platform should define `ARDUINO_ARCH_STM32`

**Error: `EEPROM.h not found`**
- Solution: Update STM32duino core to latest version (2.0.0+)

**Error: Undefined reference to `__disable_irq`**
- Solution: Ensure CMSIS is included (should be automatic with STM32duino)

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

### Runtime Issues

**Serial monitor shows garbage**
- Check baud rate matches (default 115200)
- USB CDC may require driver on Windows
- Try hardware UART instead

**Radio not working**
- Verify 3.3V power supply (nRF24 needs clean power)
- Check SPI pin connections
- Add 10µF capacitor across radio VCC/GND
- Verify CE and CS pin definitions

**EEPROM not persisting**
- EEPROM emulation requires Flash write access
- Check for debug mode preventing Flash writes
- Verify sufficient Flash space for EEPROM pages

## Performance Characteristics

### STM32F411CE Black Pill
- **CPU**: 100 MHz ARM Cortex-M4F
- **Flash**: 512KB
- **RAM**: 128KB
- **Current**: ~50mA active, <1µA standby (when implemented)
- **MySensors overhead**: ~30KB Flash, ~4KB RAM

### Benchmarks (preliminary)
- **Radio message latency**: <10ms (similar to AVR)
- **EEPROM read**: ~50µs per byte
- **EEPROM write**: ~5ms per byte (Flash write)
- **Temperature reading**: ~100µs

## Contributing

This STM32 HAL is designed for easy contribution to the main MySensors repository. When contributing:

1. Follow MySensors coding style
2. Test on multiple STM32 variants if possible
3. Document any chip-specific quirks
4. Update this README with new features

## References

- [STM32duino Core](https://github.com/stm32duino/Arduino_Core_STM32)
- [STM32duino Wiki](https://github.com/stm32duino/Arduino_Core_STM32/wiki)
- [PlatformIO STM32 Platform](https://docs.platformio.org/en/latest/platforms/ststm32.html)
- [MySensors Documentation](https://www.mysensors.org/download)
- [STM32 Reference Manuals](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html)

## License

This code is part of the MySensors project and is licensed under the GNU General Public License v2.0.

## Version History

- **v1.0.0** (2025-01-17) - Initial STM32 HAL implementation
  - Basic functionality (GPIO, SPI, EEPROM, Serial)
  - Tested on STM32F401/F411 Black Pill
  - Gateway and sensor node support
  - No sleep mode yet (planned for v1.1.0)
