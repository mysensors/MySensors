#!groovy

def call(Closure body) {
	def config = [:]
	body.resolveStrategy = Closure.DELEGATE_FIRST
	body.delegate = config
	body()

	config.pr = load(config.repository_root+'.ci/pr-toolbox.groovy')
	def linux    = load(config.repository_root+'.ci/linux.groovy')
	def arduino  = load(config.repository_root+'.ci/arduino.groovy')

	if (env.CHANGE_ID) {
		config.is_pull_request = true
		echo "Building pull request: #"+env.CHANGE_ID+"\nTarget branch: "+env.CHANGE_TARGET+"\nAuthor: "+env.CHANGE_AUTHOR+" ("+env.CHANGE_AUTHOR_EMAIL+")"
		config.git_sha = sh(returnStdout: true,
			script: """#!/bin/bash
								 cd ${config.repository_root}
								 git log -n 1 --pretty=format:'%H' refs/remotes/origin/PR-${env.CHANGE_ID}""").trim()
		// Pre-register all build statues so github shows what is going to happen
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate', 'Validating...', '${BUILD_URL}flowGraphTable/')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Butler)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Code analysis - Cppcheck)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Documentation)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - Serial GW)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - Ethernet GW)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Linux builds - MQTT GW)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsMicro - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsGW - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP32 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF52832 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF51822 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF5 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP8266 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (STM32F1 - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Uno - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Mega - Tests)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsMicro - Examples)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (MySensorsGW - Examples)', 'Not run yet...', '')
/*
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF52832 - Examples)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF51822 - Examples)', 'Not run yet...', '')
*/
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (nRF5 - Examples)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP8266 - Examples)', 'Not run yet...', '')
/*
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (ESP32 - Examples)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (STM32F1 - Examples)', 'Not run yet...', '')
*/
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Uno - Examples)', 'Not run yet...', '')
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Arduino Mega - Examples)', 'Not run yet...', '')
	} else {
		config.is_pull_request = false
		echo "Building branch: "+env.BRANCH_NAME
		config.git_sha = sh(returnStdout: true,
			script: """#!/bin/bash
								 cd ${config.repository_root}
								 git log -n 1 --pretty=format:'%H' refs/remotes/origin/${env.BRANCH_NAME}""").trim()
	}

	try {
		ansiColor('xterm') {
			if (config.is_pull_request) {
				def butler = load(config.repository_root+'.ci/butler.groovy')
				stage('Butler') {
					butler(config)
				}
			}

			stage('Preparation') {
				checkout(changelog: false, poll: false, scm: [$class: 'GitSCM', branches: [[name: '*/master']],
					extensions: [
						[$class: 'CloneOption', depth: 0, noTags: false, reference: '', shallow: true],
						[$class: 'RelativeTargetDirectory', relativeTargetDir: 'hardware/MySensors/avr']
					],
					userRemoteConfigs: [[url: 'https://github.com/mysensors/ArduinoHwAVR.git']]])
				checkout(changelog: false, poll: false, scm: [$class: 'GitSCM', branches: [[name: '*/master']],
					extensions: [
						[$class: 'CloneOption', depth: 0, noTags: false, reference: '', shallow: true],
						[$class: 'RelativeTargetDirectory', relativeTargetDir: 'hardware/MySensors/samd']
					],
					userRemoteConfigs: [[url: 'https://github.com/mysensors/ArduinoHwSAMD.git']]])
				checkout(changelog: false, poll: false, scm: [$class: 'GitSCM', branches: [[name: '*/master']],
					extensions: [
						[$class: 'CloneOption', depth: 0, noTags: false, reference: '', shallow: true],
						[$class: 'RelativeTargetDirectory', relativeTargetDir: 'hardware/MySensors/nRF5']
					],
					userRemoteConfigs: [[url: 'https://github.com/mysensors/ArduinoHwNRF5.git']]])

				config.tests    = findFiles(glob: config.library_root+'tests/**/*.ino')
				config.examples = findFiles(glob: config.library_root+'examples/**/*.ino')

			}

			parallel Doxygen: {
				if (!config.nightly_arduino_ide) {
					stage('Doxygen') {
						def doxygen  = load(config.repository_root+'.ci/doxygen.groovy')
						doxygen(config)
					}
				}
			}, CodeAnalysis: {
				if (!config.nightly_arduino_ide) {
					if (config.is_pull_request) {
						def analysis = load(config.repository_root+'.ci/static_analysis.groovy')
						stage('Cppcheck') {
							analysis.cppCheck(config)
						}
					}
				}
			}, LinuxBuilds: {
				if (!config.nightly_arduino_ide) {
					stage('LinuxGwSerial') {
						linux.buildSerial(config)
					}
					stage('LinuxGwEthernet') {
						linux.buildEthernet(config)
					}
					stage('LinuxGwMQTT') {
						linux.buildMQTT(config)
					}
				}
			}, ArduinoBuilds: {
				lock(quantity: 1, resource: 'arduinoEnv') {
					stage('MySensorsMicro (tests)') {
						arduino.buildMySensorsMicro(config, config.tests, 'Tests')
					}
					stage('MySensorsGW (tests)') {
						arduino.buildMySensorsGw(config, config.tests, 'Tests')
					}
					stage('ESP32 (tests)') {
						arduino.buildESP32(config, config.tests, 'Tests')
					}
					stage('nRF52832 (tests)') {
						arduino.buildnRF52832(config, config.tests, 'Tests')
					}
					stage('nRF51822 (tests)') {
						arduino.buildnRF51822(config, config.tests, 'Tests')
					}
					stage('nRF5 (tests)') {
						arduino.buildnRF5(config, config.tests, 'Tests')
					}
					stage('ESP8266 (tests)') {
						arduino.buildESP8266(config, config.tests, 'Tests')
					}
					stage('STM32F1 (tests)') {
						arduino.buildSTM32F1(config, config.tests, 'Tests')
					}
					stage('ArduinoUno (tests)') {
						arduino.buildArduinoUno(config, config.tests, 'Tests')
					}
					stage('ArduinoMega (tests)') {
						arduino.buildArduinoMega(config, config.tests, 'Tests')
					}
					stage('MySensorsMicro (examples)') {
						arduino.buildMySensorsMicro(config, config.examples, 'Examples')
					}
					stage('MySensorsGW (examples)') {
						arduino.buildMySensorsGw(config, config.examples, 'Examples')
					}
					// No point in building examples for nRF52832 yet
					/*
					stage('nRF52832 (examples)') {
						arduino.buildnRF52832(config, config.examples, 'Examples')
					}
					*/
					// No point in building examples for nRF51822 yet
					/*
					stage('nRF51822 (examples)') {
						arduino.buildnRF51822(config, config.examples, 'Examples')
					}
					*/
					stage('nRF5 (examples)') {
						arduino.buildnRF5(config, config.examples, 'Examples')
					}
					stage('ESP8266 (examples)') {
						arduino.buildESP8266(config, config.examples, 'Examples')
					}
					// No point in building examples for ESP32 yet
					/*
					stage('ESP32 (examples)') {
						arduino.buildESP32(config, config.examples, 'Examples')
					}
					*/
					// No point in building examples for STM32F1 yet
					/*
					stage('STM32F1 (Examples)') {
						arduino.buildSTM32F1(config, config.tests, 'Examples')
					*/
					stage('ArduinoUno (examples)') {
						arduino.buildArduinoUno(config, config.examples, 'Examples')
					}
					stage('ArduinoMega (examples)') {
						arduino.buildArduinoMega(config, config.examples, 'Examples')
					}
				}
			}, failFast: true
		}
	}	catch(ex) {
		currentBuild.result = 'FAILURE'
		throw ex
	} finally {
		if (currentBuild.result != 'SUCCESS')
		{
			config.pr.setBuildStatus(config, 'ERROR', 'Toll gate', 'Failed', '${BUILD_URL}flowGraphTable/')
			if (config.is_pull_request) {
				slackSend color: 'danger',
					message: "Failed to build <${env.CHANGE_URL}|PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE}>. Job <${env.BUILD_URL}|${env.JOB_NAME} #${env.BUILD_NUMBER}> ended with ${currentBuild.result}."
				emailext (
					subject: "PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE} failed to build",
					body: '''Greetings!<p>
					I am The Butler. My task is to help you create a pull request that fit the MySensors organizations coding style and builds for all supported platforms.<p>
					I am afraid I failed to validate your pull request. Result was '''+currentBuild.result+'''.
					<br>Please check the attached build log or <a href="${BUILD_URL}">here</a> for a hint on what the problem might be.<p>
					If you have difficulties determining the cause for the failure, feel free to ask questions <a href="${CHANGE_URL}">here</a>.<p>
					Changes:<br>
					${CHANGES}<p>
					Notable build log lines:<br>
					${BUILD_LOG_REGEX, regex="^.*?Terminated.*?$", linesBefore=0, linesAfter=0, maxMatches=5, showTruncatedLines=false, escapeHtml=true}<p>
					My personal evaluation of your pull request is available <a href="${BUILD_URL}The_20Butler_20report/butler.html">here</a>.<p>
					--<br>
					Yours sincerely, The Butler, serving the MySensors community''',
					mimeType: 'text/html', to: env.CHANGE_AUTHOR_EMAIL, attachLog: true, compressLog: false
				)
			} else {
				slackSend color: 'danger',
					message: "Failed to build branch ${env.BRANCH_NAME}. Job <${env.BUILD_URL}|${env.JOB_NAME} #${env.BUILD_NUMBER}> ended with ${currentBuild.result}."
				emailext (
					subject: "MySensors branch ${env.BRANCH_NAME} failed to build",
					body: '''I am afraid I failed to build branch ${BRANCH_NAME}. Result was '''+currentBuild.result+'''.
					<br>Please check the attached build log or <a href="${BUILD_URL}">here</a> for a hint on what the problem might be.<p>
					Changes:<br>
					${CHANGES}<p>
					Notable build log lines:<br>
					${BUILD_LOG_REGEX, regex="^.*?Terminated.*?$", linesBefore=0, linesAfter=0, maxMatches=5, showTruncatedLines=false, escapeHtml=true}<p>
					My personal evaluation of your pull request is available <a href="${BUILD_URL}The_20Butler_20report/butler.html">here</a>.<p>
					--<br>
					Yours sincerely, The Butler, serving the MySensors community''',
					mimeType: 'text/html', to: 'builds@mysensors.org', attachLog: true, compressLog: false
				)
			}
		}
		else
		{
			config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate', 'Pass', '')
		}
	}
}
return this
