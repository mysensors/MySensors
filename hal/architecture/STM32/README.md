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
- [x] Watchdog support (Independent Watchdog - IWDG)
- [x] **Low-power sleep modes** (STOP mode with RTC wake-up)
- [x] **RTC-based timekeeping** (wake-up timer for sleep intervals)
- [x] **Interrupt-based wake from sleep** (GPIO EXTI on any pin)
- [x] System reboot
- [x] Random number generation (using internal temperature sensor)
- [x] Unique device ID (96-bit STM32 UID)
- [x] CPU voltage reading (via VREFINT)
- [x] CPU temperature reading (via internal sensor)
- [x] CPU frequency reporting
- [x] Critical section (interrupt disable/restore)
- [x] RAM routing table support

### Planned 🔄
- [ ] Free memory reporting (heap analysis)
- [ ] STANDBY mode support (optional, for ultra-low-power applications)
- [ ] STM32L4-specific STOP2 mode optimization

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

## Watchdog Support

The STM32 HAL supports the Independent Watchdog (IWDG) for system reliability and crash recovery.

### Overview

- **Hardware watchdog** using STM32 IWDG peripheral
- **Maximum timeout**: ~32 seconds (hardware limitation)
- **Clock source**: Internal LSI oscillator (~32 kHz, ±40% accuracy)
- **No external components** required
- Works on all STM32 boards

### Important Notes

⚠️ **Watchdog is NOT automatically initialized** by MySensors. You must explicitly initialize and manage it in your sketch.

⚠️ **Maximum timeout is ~32 seconds**. For longer intervals (e.g., low-power sensors with 1-hour wake cycles), you must periodically wake and feed the watchdog during sleep.

⚠️ **Initialize watchdog LAST** in `setup()` after all delays and initialization to prevent premature timeout during startup.

### Timeout Calculation

The watchdog timeout depends on prescaler and reload value:

```
Timeout (seconds) = (Prescaler / 32000) × Reload
```

**Common configurations:**

| Prescaler | Reload | Timeout | Use Case |
|-----------|--------|---------|----------|
| `IWDG_PRESCALER_32` | 4000 | ~4 seconds | Normal operation |
| `IWDG_PRESCALER_128` | 4095 | ~16 seconds | Slower tasks |
| `IWDG_PRESCALER_256` | 2500 | ~20 seconds | Recommended for sleep |
| `IWDG_PRESCALER_256` | 4095 | ~32 seconds | Maximum timeout |

### Usage Example

```cpp
#include <MySensors.h>
#include "stm32f4xx_hal.h"

IWDG_HandleTypeDef hiwdg;

void initWatchdog() {
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
    hiwdg.Init.Reload = 2500;  // ~20 second timeout (256/32000 * 2500 = 20s)
    HAL_IWDG_Init(&hiwdg);
}

void setup() {
    // Initialize everything first
    // ...

    // Initialize watchdog LAST (after all delays)
    initWatchdog();
}

void loop() {
    // Feed watchdog at start of loop
    hwWatchdogReset();  // or: IWDG->KR = 0xAAAA;

    // Your sensor code
    readSensor();
    sendData();

    // Sleep in chunks, feeding watchdog during sleep
    // Must wake every <20 seconds to feed the watchdog
    watchdogSafeSleep(60000);  // Sleep for 1 minute total
}

void watchdogSafeSleep(uint32_t ms) {
    uint32_t remaining = ms;
    while (remaining > 0) {
        uint32_t chunk = min(15000, remaining);  // 15-second chunks (< 20-second timeout)
        sleep(chunk);
        hwWatchdogReset();  // Feed watchdog after each chunk
        remaining -= chunk;
    }
}
```

### Long Sleep Intervals with Watchdog

For battery-powered sensors with long sleep intervals (e.g., 1 hour), you must wake periodically to feed the watchdog:

```cpp
void loop() {
    hwWatchdogReset();

    readSensor();
    sendData();

    // Sleep for 1 hour in chunks, feeding watchdog every 15 seconds
    watchdogSafeSleep(3600000);
}

void watchdogSafeSleep(uint32_t ms) {
    uint32_t remaining = ms;
    while (remaining > 0) {
        uint32_t chunk = min(15000, remaining);  // 15s chunks (< 20s timeout)
        sleep(chunk);
        hwWatchdogReset();  // Feed watchdog after each chunk
        remaining -= chunk;
    }
}
```

**Key points:**
- Sleep in chunks smaller than watchdog timeout
- Feed watchdog between sleep chunks
- This adds brief wake-ups (~1ms every 15 seconds) but provides crash protection

**Alternative:** For ultra-low-power applications where watchdog wake-ups are unacceptable, consider:
- External watchdog IC (e.g., TPL5010 with up to 2-hour timeout)
- Software counter: Only trigger watchdog reset after N failed wake cycles

### Build Configuration

Add to `platformio.ini`:

```ini
build_flags =
    -D HAL_IWDG_MODULE_ENABLED  ; Required to enable IWDG HAL module
```

**Note**: This build flag is **required** and cannot be defined in source code. The STM32 HAL framework needs this flag during compilation.

### Detecting Watchdog Resets

Check if the last reset was caused by the watchdog:

```cpp
void setup() {
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
        // System was reset by watchdog
        __HAL_RCC_CLEAR_RESET_FLAGS();
        // Handle watchdog reset (e.g., log error, send alert)
    }

    // ... rest of setup
    initWatchdog();  // Initialize watchdog LAST
}
```

## Low-Power Sleep Support

### Overview

The STM32 HAL implements **STOP mode** for battery-powered sensor nodes, providing multi-year battery life while maintaining MySensors compatibility.

### Sleep Modes

| Mode | Sleep Current | Wake-up Time | Features | Battery Life* |
|------|---------------|--------------|----------|---------------|
| **STOP** ✅ | 10-50 µA | 1-3 ms | GPIO EXTI wake, RTC timer, state retained | **5-10 years** |
| STANDBY 🔄 | 2-4 µA | 5-10 ms | RTC timer only, state lost | 10+ years |

*Based on 2x AA batteries (2000 mAh), 5-minute reporting interval

**Currently Implemented**: STOP mode (recommended for all battery-powered MySensors nodes)

### Power Consumption

**Typical Battery-Powered Sensor** (5-minute reporting interval):

```
Average current: 30-50 µA
Battery life (2x AA): 5-10 years
Sleep current (STM32F4): 30 µA
Sleep current (STM32L4): 2-5 µA
```

### Sleep API Usage

#### Timer-Based Sleep
```cpp
void loop() {
    float temp = readTemperature();
    send(msgTemp.set(temp, 1));

    sleep(300000);  // Sleep for 5 minutes
}
```

#### Interrupt Wake-Up (Event-Driven Sensors)
```cpp
#define BUTTON_PIN PA0

void loop() {
    // Sleep until button pressed
    sleep(digitalPinToInterrupt(BUTTON_PIN), CHANGE, 0);

    // Button was pressed
    send(msgButton.set(1));
}
```

#### Combined Timer + Interrupt Wake-Up
```cpp
void loop() {
    // Sleep until button press OR 1 hour timeout
    int8_t wakeReason = sleep(digitalPinToInterrupt(BUTTON_PIN), CHANGE, 3600000);

    if (wakeReason == MY_WAKE_UP_BY_TIMER) {
        // Timed wake-up - send periodic report
        sendPeriodicReport();
    } else {
        // Button press wake-up
        handleButtonPress();
    }
}
```

### Sleep Implementation Details

**STOP Mode Characteristics**:
- ✅ **GPIO EXTI wake-up** on any pin (supports radio IRQ, sensors, buttons)
- ✅ **RTC wake-up timer** for periodic operation (1 ms to ~18 hours)
- ✅ **State retention** (SRAM and registers preserved)
- ✅ **Fast wake-up** (1-3 ms, compatible with radio timing)
- ✅ **Low power** (10-50 µA on STM32F4, 2-5 µA on STM32L4)

**Wake-up Sources**:
- RTC wake-up timer (configured automatically by `sleep(ms)`)
- GPIO EXTI interrupts (any pin, any edge)
- Watchdog timeout (if enabled)

**System Behavior**:
1. Before sleep: RTC configured, interrupts attached, SysTick suspended
2. During sleep: MCU in STOP mode (10-50 µA), peripherals stopped
3. After wake-up: System clock restored, SysTick resumed, wake source identified
4. State preserved: No reinitialization required

### Configuration Options

#### Sleep Configuration (MyConfig.h)
```cpp
// Stay on HSI (16 MHz) after wake-up for faster wake-up (default)
// Uncomment to restore full speed (84 MHz) at cost of +2 ms wake-up time
// #define MY_STM32_USE_HSE_AFTER_WAKEUP

// RTC clock source (LSE recommended for accuracy)
#define MY_STM32_RTC_CLOCK_SOURCE LSE  // Or LSI if no 32kHz crystal
```

#### Power Optimization Build Flags
```ini
[env:battery_sensor]
build_flags =
    -D MY_DISABLED_SERIAL          ; Disable serial for low power
    -D MY_TRANSPORT_WAIT_READY_MS=1  ; Don't wait for gateway
    -D MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS=2000
```

### Hardware Considerations

**For Best Battery Life**:
1. **Use STM32L4** series for ultra-low-power (2-5 µA sleep vs 30 µA on STM32F4)
2. **Add LSE crystal** (32.768 kHz) for accurate RTC timing
3. **Disable unused peripherals** (USB, debug, unused UARTs)
4. **Configure GPIO properly** (no floating pins, use pull-ups/downs)
5. **Choose efficient regulator** (low quiescent current LDO <10 µA)

**Compatible Radios**:
- **nRF24L01+**: Radio can sleep (0.9 µA), IRQ pin wakes MCU
- **RFM69/RFM95**: Radio can sleep (1-5 µA), DIO pins wake MCU

### Known Limitations

1. **Maximum sleep time**: ~18 hours (RTC wake-up timer limitation)
   - For longer intervals, use multiple sleep cycles
2. **Debug interface**: Disable in sleep for lowest power (debug keeps ~2 mA active)
3. **USB CDC**: Not compatible with sleep (use hardware UART or disable serial)
4. **STANDBY mode**: Not yet implemented (state loss, no GPIO EXTI support)

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
- **Current (active)**: ~30-50 mA
- **Current (STOP mode)**: 30-50 µA (STM32F4), 2-5 µA (STM32L4)
- **MySensors overhead**: ~30KB Flash, ~4KB RAM
- **Battery life**: 5-10 years (2x AA, 5-min reporting)

### Benchmarks
- **Radio message latency**: <10ms (comparable to AVR)
- **Wake-up time**: 1-3 ms (STOP mode)
- **EEPROM read**: ~50µs per byte
- **EEPROM write**: ~5ms per byte (Flash write)
- **Temperature reading**: ~100µs
- **Sleep current**: 30 µA typical (STM32F4 STOP mode)
- **Average current** (5-min sensor): 30-50 µA

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

- **v1.1.0** (2025-01-23) - Sleep mode and watchdog support
  - ✅ STOP mode sleep implementation
  - ✅ RTC wake-up timer (1 ms to ~18 hours)
  - ✅ GPIO EXTI interrupt wake-up (any pin, any edge)
  - ✅ Dual interrupt wake-up support
  - ✅ Independent Watchdog (IWDG) support
  - ✅ System clock reconfiguration after wake-up
  
- **v1.0.0** (2025-01-17) - Initial STM32 HAL implementation
  - Basic functionality (GPIO, SPI, EEPROM, Serial)
  - Gateway and sensor node support
  - CPU voltage and temperature reading
  - Watchdog reset function (initialization required in user sketch)
