SPIFlash
========
[![Build Status](https://travis-ci.org/LowPowerLab/SPIFlash.svg?branch=master)](https://travis-ci.org/LowPowerLab/SPIFlash)
[![GitHub release](https://img.shields.io/github/release/LowPowerLab/SPIFlash.svg)](https://github.com/LowPowerLab/SPIFlash)
[![GitHub issues](https://img.shields.io/github/issues/LowPowerLab/SPIFlash.svg)](https://github.com/LowPowerLab/SPIFlash/issues)
[![GitHub pull requests](https://img.shields.io/github/issues-pr/LowPowerLab/SPIFlash.svg)](https://github.com/LowPowerLab/SPIFlash/pulls)
[![license](https://img.shields.io/github/license/LowPowerLab/SPIFlash.svg)](https://github.com/LowPowerLab/SPIFlash/blob/master/LICENSE.txt)

Arduino/Moteino library for read/write access to SPI flash memory chips.
This works with 256byte/page SPI flash memory such as the [4MBIT W25X40CLSNIG](https://lowpowerlab.com/shop/product/72) used on [Moteino](https://www.moteino.com) for data storage and wireless programming.
<br/>
For instance a 4MBit (512Kbyte) flash chip will have 2048 pages: 256*2048 = 524288 bytes (512Kbytes).
<br/>Minimal modifications should allow chips that have different page size to work.
<br/>DEPENDS ON: Arduino native *SPI library*.
<br/>
This library was primarily developed to enable **safe** wireless programming on Moteino nodes and Moteino based applications such as the [SwitchMote](https://lowpowerlab.com/guide/switchmote/). This has been documented at [lowpowerlab](https://lowpowerlab.com/guide/moteino/wireless-programming/). [Dualoptiboot](https://github.com/LowPowerLab/DualOptiboot) (all AVR based Moteinos come with it) and [RFM69_OTA WirelessProgramming library](https://github.com/LowPowerLab/RFM69) are required to be able to wirelessly re-flash a remote Moteino.
 
### Installation
Copy the content of this library in the "Arduino/libraries/SPIFlash" folder.
<br />
To find your Arduino folder go to File>Preferences in the Arduino IDE.
<br/>
See [this tutorial](https://www.arduino.cc/en/Guide/Libraries) on installing Arduino libraries.

### License
Copyright (c) 2013-2018 by Felix Rusu <felix@lowpowerlab.com>
<br/><br/>
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
<br/><br/>
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
<br/><br/>
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
