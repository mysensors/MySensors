#!groovy
def buildLinux(config, String configuration, String key) {
	def linux_configurer              = './configure '
	def linux_configure_standard_args = '--my-rs485-serial-port=/dev/ttyS0 --my-controller-ip-address=1.2.3.4 '+
		'--my-mqtt-subscribe-topic-prefix=dummy --my-mqtt-publish-topic-prefix==dummy --my-mqtt-client-id=0 '
	def linux_builder                 = 'make '
	def linux_builder_standard_args   = '-j1'
	def linux_build_cmd               = linux_builder + linux_builder_standard_args
	def linux_configure_cmd           = linux_configurer + linux_configure_standard_args + configuration
	sh """#!/bin/bash
				printf "\\e[1m\\e[32mBuilding \\e[0musing \\e[1m\\e[36m${linux_build_cmd} \\e[0mwith \\e[1m\\e[34m${linux_configure_standard_args} ${configuration}\\e[0m\\n"
				cd ${config.library_root}
				${linux_configure_cmd}
				${linux_build_cmd} 2>> compiler_${key}.log"""
	warnings canComputeNew: false, canResolveRelativePaths: false,
		defaultEncoding: '',
		excludePattern: '''.*/EEPROM\\.h,.*/Dns\\.cpp,.*/socket\\.cpp,.*/util\\.h,.*/Servo\\.cpp,
											 .*/Adafruit_NeoPixel\\.cpp,.*/UIPEthernet.*,.*/SoftwareSerial\\.cpp,
											 .*/pins_arduino\\.h,.*/Stream\\.cpp,.*/USBCore\\.cpp,.*/Wire\\.cpp,
											 .*/hardware/esp8266.*,.*/libraries/SD/.*''',
		failedTotalAll: '', healthy: '', includePattern: '', messagesPattern: '',
		parserConfigurations: [[parserName: 'GNU Make + GNU C Compiler (gcc)', pattern: config.library_root+'compiler_'+key+'.log']],
		unHealthy: '', unstableTotalAll: '0'
	sh """#!/bin/bash
				echo "Compiler warnings/errors:"
				printf "\\e[101m"
				cat ${config.library_root}compiler_${key}.log
				printf "\\e[0m"
				rm ${config.library_root}compiler_${key}.log"""
}

def buildSerial(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - Serial GW)', 'Building...', '${BUILD_URL}flowGraphTable/')
	buildLinux(config, '--my-debug=disable --my-transport=none --my-gateway=serial', 'Serial')
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Linux builds - Serial GW)', 'Warnings found', '${BUILD_URL}warnings28Result/new')
		error 'Terminated due to warnings found'
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Linux builds - Serial GW)', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Linux builds - Serial GW)', 'Pass', '')
	}
}

def buildEthernet(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - Ethernet GW)', 'Building...', '${BUILD_URL}flowGraphTable/')
	buildLinux(config, '--my-debug=enable --my-transport=rs485 --my-gateway=ethernet', 'Ethernet')
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Linux builds - Ethernet GW)', 'Warnings found', '${BUILD_URL}warnings28Result/new')
		error 'Terminated due to warnings found'
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Linux builds - Ethernet GW)', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Linux builds - Ethernet GW)', 'Pass', '')
	}
}

def buildMQTT(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - MQTT GW)', 'Building...', '${BUILD_URL}flowGraphTable/')
	buildLinux(config, '--my-debug=disable --my-transport=none --my-gateway=mqtt', 'MQTT')
	if (currentBuild.currentResult == 'UNSTABLE') {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Linux builds - MQTT GW)', 'Warnings found', '${BUILD_URL}warnings28Result/new')
		error 'Terminated due to warnings found'
	} else if (currentBuild.currentResult == 'FAILURE') {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Linux builds - MQTT GW)', 'Build error', '${BUILD_URL}')
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Linux builds - MQTT GW)', 'Pass', '')
	}
}

return this