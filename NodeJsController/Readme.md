<h1>MySensors with NodeJS Controller and OTA bootloader on RPi</h1>
<h2>Hardware</h2>
<p>Headless Raspberry Pi model B with wired Ethernet</p>
<p>I'm connecting a monitor only for first time boot to get the IP address and then connect via Putty/xRDP</p>
<h2>Download and install latest Raspbian version on a fresh SD-Card</h2>
<p>Tutorials for this task are available on the internet (<a href="http://downloads.raspberrypi.org/raspbian_latest">download</a>, <a href="http://sourceforge.net/projects/win32diskimager">copy to SD</a>)</p>
<h2>Setup base parameters</h2>
<p>(adjust to your needs - here: RPi-IP: 10.0.1.5 / Router-IP: 10.0.1.1 / password: test / timezone: Europe,Berlin / hostname: mysensors)</p>
<ul>
<li>login as pi / raspberry
<li>sudo nano /etc/network/interfaces
<p><font color="#FF0000">#</font>iface eth0 inet dhcp<br/>
<font color="#FF0000">iface eth0 inet static<br/>
address 10.0.1.5<br/>
netmask 255.255.255.0<br/>
gateway 10.0.1.1<br/>
nameserver 10.0.1.1</font></p>
<li>sudo raspi-config
<ul>
<li>1. Expand Filesystem
<li>2. Change User Password<br/>
<p>test</p>
<li>4. Internationalisation Options
<ul>
<li>I2. Change Timezone
<p>Europe / Berlin</p>
</ul>
<li>8. Advanced Options
<ul>
<li>A2. Hostname
<p>mysensors</p>
<li>A3. Memory Split
<p>16</p>
</ul>
</ul>
<li>Finish
<li>Reboot
</ul>
<h2>Install latest updates</h2>
<ul>
<li>login as pi / test
<li>sudo apt-get update
<li>sudo apt-get upgrade
<li>sudo reboot
</ul>
<!--
<h2>Install Remote Desktop</h2>
<p>Optional… my RPi is headless and I use xRDP to connect from a Windows PC to run a graphical UI if needed (e.g. for Arduino IDE)</p>
<ul>
<li>login as pi / test
<li>sudo apt-get install xrdp
<li><i>to confirm it's working as expected try to connect from a remote computer to the RPi (e.g. from a Windows PC using the "Remote Desktop Connection" tool)</i>
</ul>
-->
<h2>Install NodeJS (here: latest stable version available for RPi: 0.10.28)</h2>
<ul>
<li>login as pi / test
<li>sudo mkdir /opt/node
<li>wget http://nodejs.org/dist/v0.10.28/node-v0.10.28-linux-arm-pi.tar.gz
<li>tar xvzf node-v0.10.28-linux-arm-pi.tar.gz
<li>rm node-v0.10.28-linux-arm-pi.tar.gz
<li>sudo cp -r node-v0.10.28-linux-arm-pi/* /opt/node
<li>rm -rf node-v0.10.28-linux-arm-pi/
<li>sudo nano /etc/profile
<p><font color="#FF0000">NODE_JS_HOME="/opt/node"<br/>
PATH="$PATH:$NODE_JS_HOME/bin"</font><br/>
export PATH</p>
<li>sudo reboot
<li><i>to confirm it's working as expected login as pi/test and run "node -v" which should return the installed node version (here: v0.10.28)</i>
</ul>
<h2>Install MongoDB</h2>
<ul>
<li>login as pi / test
<li>sudo useradd -c mongodb -d /home/mongodb -m -U mongodb
<li>sudo mkdir -p /opt/mongo/bin
<li>sudo mkdir /var/lib/mongodb
<li>sudo chown mongodb:nogroup /var/lib/mongodb
<li>sudo mkdir /var/log/mongodb
<li>sudo chown mongodb:nogroup /var/log/mongodb
<li>wget https://github.com/brice-morin/ArduPi/raw/master/mongodb-rpi/mongo/bin/mongo
<li>wget https://github.com/brice-morin/ArduPi/raw/master/mongodb-rpi/mongo/bin/mongod
<li>wget https://github.com/brice-morin/ArduPi/raw/master/mongodb-rpi/mongo/bin/mongodump
<li>wget https://github.com/brice-morin/ArduPi/raw/master/mongodb-rpi/mongo/bin/bsondump
<li>sudo mv mongo /opt/mongo/bin/
<li>sudo mv mongod /opt/mongo/bin/
<li>sudo mv mongodump /opt/mongo/bin/
<li>sudo mv bsondump /opt/mongo/bin/
<li>sudo chmod -R 775 /opt/mongo/bin
<li>sudo ln -s /opt/mongo/bin/mongod /usr/bin/mongod
<li>wget https://raw.githubusercontent.com/skrabban/mongo-nonx86/master/debian/mongodb.conf
<li>wget https://raw.githubusercontent.com/skrabban/mongo-nonx86/master/debian/init.d
<li>sudo mv mongodb.conf /etc/mongodb.conf
<li>sudo mv init.d /etc/init.d/mongodb
<li>sudo chmod 755 /etc/init.d/mongodb
<li>sudo update-rc.d mongodb defaults
<li>sudo reboot
<li><i>to confirm it's working as expected login as pi/test and run "/opt/mongo/bin/mongo" which should start the mongo shell (type "exit" to close the shell again)</i>
<li><i>if the RPi loses power without proper shutdown while mongod is running, you can't restart without manually removing the pid file first. To do so run “sudo rm /var/lib/mongodb/mongod.lock”</i>
</ul>
<!--
<h2>Install Arduino (and update to latest 1.0.5)</h2>
<ul>
<li>login as pi / test
<li>sudo apt-get install arduino
<li>wget http://arduino.googlecode.com/files/arduino-1.0.5-linux32.tgz
<li>tar zxvf arduino-1.0.5-linux32.tgz
<li>rm arduino-1.0.5-linux32.tgz
<li>cd arduino-1.0.5
<li>rm -rf hardware/tools
<li>sudo cp -ru lib /usr/share/arduino
<li>sudo cp -ru libraries /usr/share/arduino
<li>sudo cp -ru tools /usr/share/arduino
<li>sudo cp -ru hardware /usr/share/arduino
<li>sudo cp -ru examples /usr/share/doc/arduino-core 
<li>sudo cp -ru reference /usr/share/doc/arduino-core
<li>cd ~
<li>rm -rf arduino-1.0.5
</ul>
<h2>Enable programming via GPIO UART (for serial gateway only)</h2>
<ul>
<li>login as pi / test
<li>wget https://github.com/SpellFoundry/avrdude-rpi/archive/master.zip
<li>unzip master.zip
<li>rm master.zip
<li>cd avrdude-rpi-master
<li>sudo cp autoreset /usr/bin
<li>sudo cp avrdude-autoreset /usr/bin
<li>sudo mv /usr/bin/avrdude /usr/bin/avrdude-original
<li>sudo ln -s /usr/bin/avrdude-autoreset /usr/bin/avrdude
<li>cd ..
<li>rm -rf avrdude-rpi-master
</ul>
-->
<h2>Install MySensors from Git branch development (here: password: test)</h2>
<ul>
<li>login as pi / test
<li>git clone https://github.com/mysensors/Arduino.git
<li>cd Arduino
<li>git checkout development
<li>cd NodeJsController
<li>npm install
<li><i>to confirm that the NodeJsController is working as expected and connects to the local mongo DB: "node NodeJsController.js" which should start the controller, connect to the database and run into an error connecting to the gateway. Stop node again via Ctrl-c</i>
</ul>
<h2>Start NodeJsController on boot</h2>
<ul>
<li>cd ~/Arduino/NodeJsController
<li>chmod 755 *.sh
<li>sudo cp MySensorsInitScript.sh /etc/init.d/mysensors
<li>sudo mkdir -p /usr/local/var/run
<li>sudo chmod -R 777 /usr/local/var/run
<li>sudo update-rc.d mysensors defaults
<li>sudo reboot
<li><i>to confirm that NodeJsController is starting at boot, check the log file /home/pi/logs/NodeJsController.log</i>
</ul>
<!--
<h2>Configure Arduino</h2>
<ul>
<li>start graphical UI (startx on RPi or via RDP)
<li>start Arduino IDE
<li>File - Preferences
<ul>
<li>set sketchbook location to /home/pi/Arduino
<li>enable verbose output for compilation
<li>enable verbose output for upload
</ul>
<li>Close Arduino IDE
</ul>
-->

<h1>Serial Gateway</h1>
<h2>Hardware</h2>
<ul>
<li>as described for the standard Serial Gateway - using the standard Serial Gateway sketch with DEBUG (in MyConfig.h) disabled
<li>I'm using the version with inclusion mode button and LEDs based on no Arduino but a plain ATmega328p on a breadboard
<li>Connect directly to the serial GPIO pins of the RPi:
<p>Arduino GND <-> Raspberry Pi GND (pin 6)<br/>
Arduino RX <-> Raspberry Pi TX (pin 8)<br/>
Arduino TX <-> Raspberry Pi RX (pin 10)<br/>
Arduino RESET <-> Raspberry Pi GPIO22 (pin 15)</p>
<li>Right now I'm using a separate power supply for the serial gateway instead of the 3.3V or 5V pins of the RPi because I ran in some issues with power consumption during programming
</ul>
<h2>Disable default use of RPi serial port</h2>
<ul>
<li>sudo nano /etc/inittab
<p><font color="#FF0000">#</font>T0:23:respawn:/sbin/getty -L ttyAMA0 115200 vt100</p>
<li>sudo nano /boot/cmdline.txt - remove part of the file:
<p><font color="#FF0000"><strike>console=ttyAMA0,115200</strike></font></p>
<li>cd /dev
<li>sudo ln -s ttyAMA0 ttyUSB9
<li>sudo apt-get install minicom
<li>sudo reboot
</ul>
<!--
<h2>Flash (assuming that the ATmega has a valid Arduino bootloader installed)</h2>
<ul>
<li>login as pi / test
<li>start Arduino IDE
<ul>
<li>Tools - Board - Arduino - Arduino Pro or Pro Mini (5V, 16 MHz) w/ ATmega328
<li>Tools - Serial Port - /dev/ttyUSB9
<li>File - Sketchbook - SerialGateway
<li>File - Upload
</ul>
</ul>
-->
<h2>Setup NodeJsController</h2>
<ul>
<li>nano ~/Arduino/NodeJsController/NodeJsController.js
<p><font color="#FF0000">//</font>const gwType = 'Ethernet';<br/>
<font color="#FF0000">//</font>const gwAddress = '10.0.1.99';<br/>
<font color="#FF0000">//</font>const gwPort = 9999;<br/>
<font color="#FF0000"><strike>//</strike></font>const gwType = 'Serial';<br/>
<font color="#FF0000"><strike>//</strike></font>const gwPort = '<font color="#FF0000">/dev/ttyAMA0</font>';<br/>
<font color="#FF0000"><strike>//</strike></font>const gwBaud = 115200;</p>
<li>sudo service mysensors restart (at this point stop/restart doesn't work. use "sudo killall node" followed by "sudo service mysensors start" instead)
</ul>

<h1>Ethernet Gateway</h1>
<h2>Hardware</h2>
<ul>
<li>as described for the standard Ethernet Gateway - using the standard Ethernet Gateway sketch with DEBUG (in MyConfig.h) disabled
<li>I'm using the version with inclusion mode button and LEDs based on no Arduino but a plain ATmega328p on a breadboard and the ENC28J60 Ethernet module
<li>here: 10.0.1.99 / port: 9999
</ul>
<!--
<h2>Adjust Sourcecode (here: address 10.0.1.99 / port: 9999)</h2>
<ul>
<li>[I you are running into compilation errors due to missing files in the utility directory, do the following]
cp -R ~/Arduino/libraries/UIPEthernet/src/utility ~/Arduino/libraries/UIPEthernet/utility
<li>nano ~/Arduino/libraries/MySensors/MyConfig.h
<p><font color="#FF0000">//</font>#define DEBUG</p>
<li>start Arduino IDE
<ul>
<li>File - Sketchbook - EthernetGateway
<li>Edit source code
<p>#define IP_PORT 9999<br/>
IPAddress myIp (10, 0, 1, 99);</p>
<li>Optional : adjust source code to given hardware
<ul>
<li>Correct include for Ethernet module type (if not using ENC28J60)
<li>Switch MyGateway constructor if using inclusion mode button and LEDs
</ul>
<li>File - Save
</ul>
<h2>Flash (assuming that the ATmega has a valid Arduino bootloader installed)</h2>
<ul>
<li>Connect ethernet gateway to RPi (setup as above for serial gateway or via USB FTDI cable
<li>login as pi / test
<li>start Arduino IDE
<ul>
<li>Tools - Board - Arduino - Arduino Pro or Pro Mini (5V, 16 MHz) w/ ATmega328
<li>Tools - Serial Port - /dev/ttyUSB9 (or /dev/ttyUSB0 if connecting through USB cable)
<li>File - Sketchbook - EthernetGateway
<li>File - Upload
</ul>
</ul>
-->
<h2>Setup NodeJsController</h2>
<ul>
<li>nano ~/Arduino/NodeJsController/NodeJsController.js
<p>const gwAddress = '<font color="#FF0000">10.0.1.99</font>';</p>
<li>sudo service mysensors restart (at this point stop/restart doesn't work. use "sudo killall node" followed by "sudo service mysensors start" instead)
</ul>

<h1>Sensor Node Bootloader</h1>
<h2>Hardware</h2>
<ul>
<li>as described for a simple Sensor Node
<li>here: no Arduino but a plain ATmega328p on a breadboard
<li>using DallasTemperature example (one sensor connected to pin 3)
</ul>
<h2>Compile Bootloader (can be skipped but good to see if everything is working)</h2>
<ul>
<li>cd ~/Arduino/Bootloader
<li>make
<li><i>this should compile the bootloader and produce three files in the same directory: MyOtaBootloader.o / MyOtaBootloader.elf / MyOtaBootloader.hex</i>
</ul>
<h2>Flash Bootloader</h2>
<ul>
<li>cd ~/Arduino/Bootloader
<li>nano Makefile
<li>adjust avrdude settings to the available programmer. In my case (mySmartUSB MK3 programmer temporarily connected to RPi USB port):
<p>ISP_PORT = /dev/ttyUSB0<br/>
ISP_SPEED = 115200<br/>
ISP_PROTOCOL = stk500v2<br/>
ISP_MCU = m328p</p>
<li>make load 
</ul>

<h1>End-to-End Test</h1>
<h2>Add Firmware to Controller (here: DallasTemperatureSensor)</h2>
<ul>
<li>nano ~/Arduino/NodeJsController/NodeJsController.js
<p>const fwHexFiles = [ <font color="#FF0000">'../libraries/MySensors/examples/DallasTemperatureSensor/DallasTemperatureSensor.ino'</font> ];<br/>
const fwDefaultType = <font color="#FF0000">0</font>;</p>
<li>sudo service mysensors restart (at this point stop/restart doesn't work. use "sudo killall node" followed by "sudo service mysensors start" instead)
</ul>
<h2>Flash Firmware OTA</h2>
<ul>
<li>tail -f ~/logs/NodeJsController.log
<li>Power up sensor node with OTA bootloader
</ul>
<h2>Check functionality</h2>
<ul>
<li>in addition to watching the log file (tail command above) you can connect to the mongo db using a client (e.g. <a href="http://robomongo.org">Robomongo</a>) and check if new values are written to the database as a real end-to-end test once the bootloader finished uploading the firmware
</ul>
