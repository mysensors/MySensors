#!groovy

def buildArduino(config, String buildFlags, String sketch, String key) {
    def cli = config.arduino_cli ?: 'arduino-cli'

    // Per-key build dir (so different boards don’t step on each other)
    def build_path       = "build/${key}"
    def build_path_cmd       = "--build-path ${build_path}"

    // Optional parallel jobs (defaults to 4 if nothing is set)
    def jobs = config.arduino_jobs ?: '4'
    def jobsOpt = jobs ? "--jobs ${jobs}" : ""

    // If the board-specific flags already contain --warnings, don't add another one
    def hasWarnings = (buildFlags =~ /--warnings\b/).find()
    def warningsOpt = hasWarnings ? '' : '--warnings all'
    
    def libOpt = ""
    if (config.library_root) {
        libOpt = "--library \"${config.library_root}\""
    }

    def build_cmd = "${cli} compile ${buildFlags} ${build_path_cmd} ${warningsOpt} ${libOpt} ${jobsOpt}"

    sh """#!/bin/bash
    set -e
    
    printf "\\e[1m\\e[32mBuilding \\e[34m${sketch} \\e[0musing \\e[1m\\e[36m${build_cmd}\\e[0m\\n"
    
    mkdir -p "${build_path}"
    
    ${build_cmd} "${sketch}" 2>> compiler_${key}.log
    """
}

def parseWarnings(String key) {
    def logFile = "compiler_${key}.log"

    warnings canResolveRelativePaths: false, canRunOnFailed: true, categoriesPattern: '',
        defaultEncoding: '',
        excludePattern: '''.*/EEPROM\\.h,.*/Dns\\.cpp,.*/socket\\.cpp,.*/util\\.h,.*/Servo\\.cpp,
                           .*/Adafruit_NeoPixel\\.cpp,.*/UIPEthernet.*,.*/SoftwareSerial\\.cpp,.*/PJON/.*,
                           .*/pins_arduino\\.h,.*/Stream\\.cpp,.*/USBCore\\.cpp,.*/hardware/.*,.*/libraries/.*''',
        healthy: '', includePattern: '', messagesPattern: '',
        parserConfigurations: [[parserName: 'Arduino/AVR', pattern: logFile]],
        unHealthy: '', unstableNewAll: '0', unstableTotalAll: '0'

    sh """#!/bin/bash
        echo "Compiler warnings/errors:"
        printf "\\e[101m"
        if [ -f "${logFile}" ]; then
            cat "${logFile}"
            rm "${logFile}"
        else
            echo "Log file ${logFile} not found (build may have failed before compilation)."
        fi
        printf "\\e[0m"
    """
}

// ---------------------------
// Board-specific wrappers
// ---------------------------

def buildMySensorsMicro(config, sketches, String key) {
	def fqbn = '--fqbn MySensors:avr:MysensorsMicro:cpu=1Mhz'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsMicro - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayGSMMQTTClient/GatewayGSMMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_MySensorsMicro')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (MySensorsMicro - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_MySensorsMicro')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (MySensorsMicro - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (MySensorsMicro - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (MySensorsMicro - '+key+')', 'Pass', '')
	}
}

def buildMySensorsGw(config, sketches, String key) {
	def fqbn = '--fqbn MySensors:samd:mysensors_gw_native'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsGW - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/BatteryPoweredSensor/BatteryPoweredSensor.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayGSMMQTTClient/GatewayGSMMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewaySerialRS485/GatewaySerialRS485.ino' &&
					sketch.path != config.library_root+'examples/MotionSensorRS485/MotionSensorRS485.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_MySensorsGW')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (MySensorsGW - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_MySensorsGW')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (MySensorsGW - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (MySensorsGW - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (MySensorsGW - '+key+')', 'Pass', '')
	}
}

def buildArduinoUno(config, sketches, String key) {
	def fqbn = '--fqbn arduino:avr:uno'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Uno - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_ArduinoUno')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Arduino Uno - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_ArduinoUno')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Arduino Uno - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Arduino Uno - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Arduino Uno - '+key+')', 'Pass', '')
	}
}

def buildArduinoMega(config, sketches, String key) {
	def fqbn = '--fqbn arduino:avr:mega:cpu=atmega2560'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Mega - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_ArduinoMega')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Arduino Mega - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_ArduinoMega')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		// fixed: removed duplicate config argument
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Arduino Mega - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Arduino Mega - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Arduino Mega - '+key+')', 'Pass', '')
	}
}

def buildSTM32F1(config, sketches, String key) {
	def fqbn = '--fqbn STMicroelectronics:stm32:GenF1:pnum=BLUEPILL_F103C8,upload_method=swdMethod,xserial=generic,usb=none,xusb=FS,opt=osstd,dbg=none,rtlib=nano'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (STM32F1 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_STM32F1')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (STM32F1 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_STM32F1')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (STM32F1 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (STM32F1 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (STM32F1 - '+key+')', 'Pass', '')
	}
}

def buildSTM32F4(config, sketches, String key) {
	def fqbn = '--fqbn STMicroelectronics:stm32:GenF4:pnum=BLACKPILL_F411CE,upload_method=swdMethod,xserial=generic,usb=none,xusb=FS,opt=osstd,dbg=none,rtlib=nano'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (STM32F4 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_STM32F4')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (STM32F4 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_STM32F4')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (STM32F4 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (STM32F4 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (STM32F4 - '+key+')', 'Pass', '')
	}
}

def buildESP8266(config, sketches, String key) {
	def fqbn = '--fqbn esp8266:esp8266:generic:xtal=80,vt=flash,exception=disabled,ResetMethod=ck,CrystalFreq=26,FlashFreq=40,FlashMode=dout,eesz=512K,led=2,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP8266 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/BatteryPoweredSensor/BatteryPoweredSensor.ino' &&
					sketch.path != config.library_root+'examples/CO2Sensor/CO2Sensor.ino' &&
					sketch.path != config.library_root+'examples/DustSensorDSM/DustSensorDSM.ino' &&
					sketch.path != config.library_root+'examples/GatewaySerialRS485/GatewaySerialRS485.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100/GatewayW5100.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100MQTTClient/GatewayW5100MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayGSMMQTTClient/GatewayGSMMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/MotionSensorRS485/MotionSensorRS485.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino' &&
					sketch.path != config.library_root+'examples/SoilMoistSensor/SoilMoistSensor.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_ESP8266')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (ESP8266 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_ESP8266')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (ESP8266 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (ESP8266 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (ESP8266 - '+key+')', 'Pass', '')
	}
}

def buildESP32(config, sketches, String key) {
	// Note: override global --warnings with "default" for ESP32
	def fqbn = '--fqbn esp32:esp32:esp32:PartitionScheme=default,FlashMode=qio,FlashFreq=80,FlashSize=4M,UploadSpeed=921600,DebugLevel=none --warnings default'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP32 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/BatteryPoweredSensor/BatteryPoweredSensor.ino' &&
					sketch.path != config.library_root+'examples/BinarySwitchSleepSensor/BinarySwitchSleepSensor.ino' &&
					sketch.path != config.library_root+'examples/CO2Sensor/CO2Sensor.ino' &&
					sketch.path != config.library_root+'examples/DustSensor/DustSensor.ino' &&
					sketch.path != config.library_root+'examples/DustSensorDSM/DustSensorDSM.ino' &&
					sketch.path != config.library_root+'examples/EnergyMeterPulseSensor/EnergyMeterPulseSensor.ino' &&
					sketch.path != config.library_root+'examples/LightSensor/LightSensor.ino' &&
					sketch.path != config.library_root+'examples/LogOTANode/LogOTANode.ino' &&
					sketch.path != config.library_root+'examples/MotionSensor/MotionSensor.ino' &&
					sketch.path != config.library_root+'examples/MotionSensorRS485/MotionSensorRS485.ino' &&
					sketch.path != config.library_root+'examples/PassiveNode/PassiveNode.ino' &&
					sketch.path != config.library_root+'examples/GatewaySerialRS485/GatewaySerialRS485.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100/GatewayW5100.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100MQTTClient/GatewayW5100MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayGSMMQTTClient/GatewayGSMMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino' &&
					sketch.path != config.library_root+'examples/MotionSensorRS485/MotionSensorRS485.ino' &&
					sketch.path != config.library_root+'examples/SoilMoistSensor/SoilMoistSensor.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_ESP32')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (ESP32 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_ESP32')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (ESP32 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (ESP32 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (ESP32 - '+key+')', 'Pass', '')
	}
}

def buildnRF52(config, sketches, String key) {
	def fqbn = '--fqbn sandeepmistry:nRF5:Generic_nRF52832:softdevice=none,lfclk=lfxo'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF5 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		sketches.each { sketch ->
			if (sketch.path != config.library_root+'examples/BatteryPoweredSensor/BatteryPoweredSensor.ino' &&
					sketch.path != config.library_root+'examples/CO2Sensor/CO2Sensor.ino' &&
					sketch.path != config.library_root+'examples/DustSensorDSM/DustSensorDSM.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266/GatewayESP8266.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266MQTTClient/GatewayESP8266MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266SecureMQTTClient/GatewayESP8266SecureMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayGSMMQTTClient/GatewayGSMMQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP8266OTA/GatewayESP8266OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32/GatewayESP32.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32OTA/GatewayESP32OTA.ino' &&
					sketch.path != config.library_root+'examples/GatewayESP32MQTTClient/GatewayESP32MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/GatewaySerialRS485/GatewaySerialRS485.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100/GatewayW5100.ino' &&
					sketch.path != config.library_root+'examples/GatewayW5100MQTTClient/GatewayW5100MQTTClient.ino' &&
					sketch.path != config.library_root+'examples/MotionSensorRS485/MotionSensorRS485.ino' &&
					sketch.path != config.library_root+'examples/SensebenderGatewaySerial/SensebenderGatewaySerial.ino') {
				buildArduino(config, fqbn, sketch.path, key+'_nRF52')
			}
		}
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF52 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_nRF52')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (nRF52 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF52 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (nRF52 - '+key+')', 'Pass', '')
	}
}

def buildnRF52832(config, sketches, String key) {
	def fqbn = '--fqbn MySensors:nRF5:MyBoard_nRF52832:bootcode=none,lfclk=lfxo,reset=notenable'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF52832 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		buildArduino(config, fqbn, 'hardware/MySensors/nRF5/libraries/MyBoardNRF5/examples/MyBoardNRF5/MyBoardNRF5.ino', key+'_nRF52832')
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF52832 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_nRF52832')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (nRF52832 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF52832 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (nRF52832 - '+key+')', 'Pass', '')
	}
}

def buildnRF51822(config, sketches, String key) {
	def fqbn = '--fqbn MySensors:nRF5:MyBoard_nRF51822:chip=xxaa,bootcode=none,lfclk=lfxo'
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF51822 - '+key+')', 'Building...', '${BUILD_URL}flowGraphTable/')
	try {
		buildArduino(config, fqbn, 'hardware/MySensors/nRF5/libraries/MyBoardNRF5/examples/MyBoardNRF5/MyBoardNRF5.ino', key+'_nRF51822')
	} catch (ex) {
		echo "Build failed with: "+ ex.toString()
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF51822 - '+key+')', 'Build error', '${BUILD_URL}')
		throw ex
	} finally {
		parseWarnings(key+'_nRF51822')
	}
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (nRF51822 - '+key+')', 'Warnings found', '${BUILD_URL}warnings2Result/new')
		if (config.is_pull_request) {
			error 'Terminated due to warnings found'
		}
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (nRF51822 - '+key+')', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (nRF51822 - '+key+')', 'Pass', '')
	}
}

return this
