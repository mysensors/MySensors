Name    : Sleep_n0m1 Library                         
Author  : Noah Shibley, Michael Grant, NoMi Design Ltd. http://n0m1.com                       
Date    : July 10th 2011                                    
Version : 0.1                                              
Notes   : Arduino Library to place the arduino into sleep mode for 
          a specific length of time, or a specific number of sleep cycles 
          Some of this code comes from "Cloudy" on the arduino forum
	      http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1292898715   
	 
	  Refer to the examples for how to use this library.  
			
Dependencies:	
	none
	
	
			
List of Functions:

Function: idleMode
Description: sets the Arduino into idle Mode sleep, 
the least power saving, The idle mode stops the MCU
but leaves peripherals and timers running. 
------------------------------------------------------	
Function: adcMode
Description: sets the Arduino into adc Mode sleep, 
This mode makes the MCU enter ADC Noise Reduction mode,
stopping the CPU but allowing the ADC, the external interrupts,
the 2-wire Serial Interface address watch, Timer/Counter2
and the Watchdog to continue operating
------------------------------------------------------
Function: pwrSaveMode
Description: sets the Arduino into power Save Mode sleep,
The timer crystal will continue to operate in this mode, 
Timer2 still active.
------------------------------------------------------
Function: extStandbyMode
Description: sets the Arduino into extStandby Mode sleep,
This mode is identical to Power-save with the exception
that the Oscillator is kept running for fast wake up
------------------------------------------------------ 
Function: standbyMode
Description: sets the Arduino into standby Mode sleep,
This mode is identical to Power-down with the exception
that the Oscillator is kept running for fast wake up
------------------------------------------------------
Function: pwrDownMode
Description: sets the Arduino into power Down Mode sleep,
The most power saving, all systems are powered down 
except the watch dog timer and external reset
------------------------------------------------------
Function: sleepDelay
Description: Works like the Arduino delay function, sets the
Arduino into sleep mode for a specified time.
Parameters: (unsigned long) time in ms of the sleep cycle
------------------------------------------------------  
Function: sleepDelay
Description: Works like the Arduino delay function, sets the
Arduino into sleep mode for a specified time.
Parameters: (unsigned long) time in ms of the sleep cycle
            (boolean) prevents the Arduino from entering sleep
------------------------------------------------------
Function: setCalibrationInterval
Description: the WDT needs to be calibrated against timer 0 
periodically to keep the sleep time accurate. Default calibration
occurs every 100 wake/sleep cycles. recalibrate too often will
waste power and too rarely will make the sleep time inaccurate. 
Parameters: (int) set the # of wake/sleep cycles between calibrations
------------------------------------------------------
Function: sleepInterrupt
Description: set the Arduino into sleep mode until an interrupt is
triggered. The interrupts are passed in as parameters
Parameters: (int) interrupt value, 0, 1, etc, see attachinterrupt()
	    (int) mode of trigger, LOW,RISING,FALLING,CHANGE