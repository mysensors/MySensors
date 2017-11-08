This code is based on (arduino-NVM)[https://github.com/d00616/arduino-NVM]. If you change code, please create a Pull Request for arduino-NVM to keep code synchronized.

# arduino-NVM

[![Build Status](https://travis-ci.org/d00616/arduino-NVM.svg?branch=master)](https://travis-ci.org/d00616/arduino-NVM)

This library allows the usage of internal Flash memory. To enhance the limited erase cycles a VirtualPage layer is available. On top of VirtualPage, there is an NVRAM class to allow a lot of writes by using a log-based storage.

For Arduino compatibility, a subset of avr/eeprom.h functionality and a complete port of EEPROM.h is provided.

Accessing bytes via NVRAM or EEPROM is faster than an AVR controller until the internal log is full. At this point, a new page must build. This process takes up to 3400ms (nRF51) or 1300ms (nRF52) depending on your hardware and the highest written address.

To find out more about timing, please run "test_all" example.

_This code is not compatible with any SoftDevice. You have to use the [radio notification](https://devzone.nordicsemi.com/tutorials/14/radio-notification/) and VirtualPage.clean_up()/NVRAM.write_prepare(NUMBER) to ensure that writes are only used in a time without radio activation._

## Flash.h

This class is the hardware abstraction to the Flash controller. Please look into Flash.h for a more detailed description.

Please read the documentation of your microcontroller to find out limitations about writing into flash. You can use the FLASH_... defines in your code to take care about quirks.

## VirtualPage.h

This class provides manages Flash pages. This helps you to reach more erase cycles and handle page faults. The underlying Flash page needs to hold some metadata so a VirtualPage is some bytes smaller than 4k. The size can differ between different hardware.

If you need to allocate VirtualPages in performance critical situations, call VirtualPage.clean_up(), when you have a time slot of more than 100ms.

For VirtualPages the last 16k(nRF51) or 32k(nRF52) are used. This allows the same number of erase cycles on both platforms.

## NVRAM.h

This class provides a 3072 bytes large memory. You can access this memory in a random order without needing to take care of the underlying flash architecture. This class is stateless, this means there is nothing cached in RAM. With every access, the data structure is parsed. This saves RAM and avoids conflicts when you have more than one instance of NVRAM class in your code.

To reach a maximum of write cycles and performance, place all your data at the beginning of the memory. This allows a maximum of write cycles.

When you only use the first 8 Bytes of the NVRAM, you have 5,100,000 write cycles per byte. If you use all 3072 bytes, you have only 3,300 write cycles per byte.

For your own calculation of write cycles, you can calculate the sum of available writes with: (VirtualPage.length()-4-HIGHEST_ADDRESS/4)*40,000

Reading or writing the NVRAM is fast until the internal log is full. On nRF51 you can calculate with 1.2ms and on nRF52 with 0,5ms. If the log is full a new VirtualPage is allocated and written. This procedure can take a time of 3400ms (nRF51) or 1300ms (nRF52).

If you use this code in performance critical situations. Use NVRAM.write_prepare(NUMBER) before to guarantee a fast write for the given number of bytes.

## EEPROM.h and avr_eeprom.h

Both libraries are for Arduino compatibility. Instead of avr/eeprom.h, you have to include avr_eeprom.h. This file maps a limited set of functions to NVRAM.

The EEPROM.h is fully compatible with the AVR version without committing changes.

If you use one of both files, please keep in mind that writing a single byte is fast until the log becomes full. In that case, a single write operation can take up to 3400ms (nRF51) or 1300ms (nRF52).
