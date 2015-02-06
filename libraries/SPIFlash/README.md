SPIFlash
========
Arduino/Moteino library for read/write access to SPI flash memory chips.
This works with 256byte/page SPI flash memory such as the 4MBIT W25X40CLSNIG used on (Moteinos)[www.moteino.com] for data storage and wireless programming.
<br/>
For instance a 4MBit (512Kbyte) flash chip will have 2048 pages: 256*2048 = 524288 bytes (512Kbytes).
<br/>Minimal modifications should allow chips that have different page size to work.
<br/>DEPENDS ON: Arduino *SPI library*.
<br/>
This library was primarily developed to enable **safe** wireless programming on Moteino nodes and Moteino based applications such as the SwitchMote. This has been documented at [lowpowerlab](http://lowpowerlab.com/blog/category/moteino/wireless-programming/). [Dualoptiboot](https://github.com/LowPowerLab/DualOptiboot) (all Moteinos come with it) and [WirelessProgramming library](https://github.com/LowPowerLab/WirelessProgramming) are required to be able to wirelessly re-flash a remote Moteino.
 
###Installation
Copy the content of this library in the "Arduino/libraries/SPIFlash" folder.
<br />
To find your Arduino folder go to File>Preferences in the Arduino IDE.
<br/>
See [this tutorial](http://learn.adafruit.com/arduino-tips-tricks-and-techniques/arduino-libraries) on Arduino libraries.

###License
Copyright (c) 2013 by Felix Rusu <felix@lowpowerlab.com>
<br/>
This library is free software; you can redistribute it and/or modify it under the terms of either the GNU General Public License version 2 or the GNU Lesser General Public License version 2.1, both as published by the Free Software Foundation.
<br/>
This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
