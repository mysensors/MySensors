#
#  Arduino Zero OpenOCD script.
#
#  Copyright (c) 2014-2015 Arduino LLC. All right reserved.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

# Define 'reset' command
define reset

info reg

break main

# End of 'reset' command
end

target remote | openocd -c "interface cmsis-dap" -c "set CHIPNAME at91samd21g18" -f target/at91samdXX.cfg -c "gdb_port pipe; log_output openocd.log"
