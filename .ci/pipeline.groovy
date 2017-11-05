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
		echo "Building pull request: #"+env.CHANGE_ID+"\nTarget branch: "+env.CHANGE_TARGET
		config.git_sha = sh(returnStdout: true,
			script: """#!/bin/bash
								 cd ${config.repository_root}
								 git log -n 1 --pretty=format:'%H' refs/remotes/origin/PR-${env.CHANGE_ID}""").trim()
	} else {
		config.is_pull_request = false
		echo "Building branch: "+env.BRANCH_NAME
		config.git_sha = sh(returnStdout: true,
			script: """#!/bin/bash
								 cd ${config.repository_root}
								 git log -n 1 --pretty=format:'%H' refs/remotes/origin/${env.BRANCH_NAME}""").trim()
		config.pr.setBuildStatus(config, 'PENDING', 'Toll gate', 'Validating...', '${BUILD_URL}flowGraphTable/')
	}

	try {
		ansiColor('xterm') {
			if (config.is_pull_request) {
				def gitler = load(config.repository_root+'.ci/gitler.groovy')
				stage('Gitler') {
					gitler(config)
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
						arduino.buildEsp8266(config, config.tests, 'Tests')
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
						arduino.buildEsp8266(config, config.examples, 'Examples')
					}
					// No point in building examples for STM32F1 yet
					/*
					stage('STM32F1 (Examples)') {
						arduino.buildSTM32F1(config, config.tests, 'Examples')
					}
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
					message: "Job '${env.JOB_NAME} <${env.BUILD_URL}|#${env.BUILD_NUMBER}> <${env.CHANGE_URL}|PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE}>' failed with result ${currentBuild.result}."
				emailext (
					subject: "Job '${env.JOB_NAME} #${env.BUILD_NUMBER}' failed",
					body: """Job '${env.JOB_NAME} <a href="${env.BUILD_URL}">#${env.BUILD_NUMBER}</a> (<a href="${env.CHANGE_URL}">PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE}</a>)' ended with result ${currentBuild.result}.
					<br>Check attached console output or <a href="${env.BUILD_URL}">here</a> for a hint on what the problem might be.""",
					mimeType: 'text/html', to: env.CHANGE_AUTHOR_EMAIL, attachLog: true, compressLog: false
				)
			} else {
				slackSend color: 'danger',
					message: "Job '${env.JOB_NAME} <${env.BUILD_URL}|#${env.BUILD_NUMBER}> ${env.BRANCH_NAME}]' failed with result ${currentBuild.result}."
				emailext (
					subject: "Job '${env.JOB_NAME} #${env.BUILD_NUMBER} ${env.BRANCH_NAME}' failed",
					body: """Job '${env.JOB_NAME} <a href="${env.BUILD_URL}">#${env.BUILD_NUMBER}</a> (${env.BRANCH_NAME})' ended with result ${currentBuild.result}.
					<br>Check attached console output or <a href="${env.BUILD_URL}">here</a> for a hint on what the problem might be.""",
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