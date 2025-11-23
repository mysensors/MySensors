#!groovy

def call(Closure body) {
    // Config setup
    def config = [:]
    body.resolveStrategy = Closure.DELEGATE_FIRST
    body.delegate        = config
    body()

    config.pr      = load("${config.repository_root}.ci/pr-toolbox.groovy")
    def linux      = load("${config.repository_root}.ci/linux.groovy")
    def arduino    = load("${config.repository_root}.ci/arduino.groovy")

    // PR / branch metadata
    if (env.CHANGE_ID) {
        config.is_pull_request = true

        echo """Building pull request: #${env.CHANGE_ID}
Target branch: ${env.CHANGE_TARGET}
Author: ${env.CHANGE_AUTHOR} (${env.CHANGE_AUTHOR_EMAIL})"""

        config.git_sha = resolveGitSha(
            config,
            "refs/remotes/origin/PR-${env.CHANGE_ID}"
        )

        preRegisterBuildStatuses(config)
    } else {
        config.is_pull_request = false
        echo "Building branch: ${env.BRANCH_NAME}"

        config.git_sha = resolveGitSha(
            config,
            "refs/remotes/origin/${env.BRANCH_NAME}"
        )
    }

    // Pipeline body
    try {
        ansiColor('xterm') {

            // Butler only runs for PRs
            if (config.is_pull_request) {
                def butler = load("${config.repository_root}.ci/butler.groovy")
                stage('Butler') {
                    butler(config)
                }
            }

            stage('Preparation') {
                checkoutHardware()
                config.tests    = findFiles(glob: "${config.library_root}tests/**/*.ino")
                config.examples = findFiles(glob: "${config.library_root}examples/**/*.ino")
            }

            parallel(
                Doxygen: {
                    if (!config.nightly_arduino_ide) {
                        stage('Doxygen') {
                            def doxygen = load("${config.repository_root}.ci/doxygen.groovy")
                            doxygen(config)
                        }
                    }
                },
                CodeAnalysis: {
                    if (!config.nightly_arduino_ide && config.is_pull_request) {
                        def analysis = load("${config.repository_root}.ci/static_analysis.groovy")
                        stage('Cppcheck') {
                            analysis.cppCheck(config)
                        }
                    }
                },
                LinuxBuilds: {
                    if (!config.nightly_arduino_ide) {
                        stage('LinuxGwSerial')   { linux.buildSerial(config)   }
                        stage('LinuxGwEthernet') { linux.buildEthernet(config) }
                        stage('LinuxGwMQTT')     { linux.buildMQTT(config)     }
                    }
                },
                ArduinoBuilds: {
                    lock(quantity: 1, resource: 'arduinoEnv') {
                        runArduinoBuilds(arduino, config)
                    }
                },
                failFast: true
            )
        }
    } catch (ex) {
        throw ex
    } finally {
        // currentResult is always non-null (SUCCESS / FAILURE / UNSTABLE / ABORTED)
        def result = currentBuild.currentResult

        if (result != 'SUCCESS') {
            config.pr.setBuildStatus(
                config,
                'ERROR',
                'Toll gate',
                'Failed',
                '${BUILD_URL}flowGraphTable/'
            )

            if (config.is_pull_request) {
                slackSend(
                    color: 'danger',
                    message: "Failed to build <${env.CHANGE_URL}|PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE}>. " +
                             "Job <${env.BUILD_URL}|${env.JOB_NAME} #${env.BUILD_NUMBER}> ended with ${result}."
                )
                emailext(
                    subject: "PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE} failed to build",
                    body: '''Greetings!<p>
                    I am The Butler. My task is to help you create a pull request that fit the MySensors organizations coding style and builds for all supported platforms.<p>
                    I am afraid I failed to validate your pull request. Result was ''' + result + '''.
                    <br>Please check the attached build log or <a href="${BUILD_URL}">here</a> for a hint on what the problem might be.<p>
                    If you have difficulties determining the cause for the failure, feel free to ask questions <a href="${CHANGE_URL}">here</a>.<p>
                    Changes:<br>
                    ${CHANGES}<p>
                    Notable build log lines:<br>
                    ${BUILD_LOG_REGEX, regex="^.*?Terminated.*?$", linesBefore=0, linesAfter=0, maxMatches=5, showTruncatedLines=false, escapeHtml=true}<p>
                    My personal evaluation of your pull request is available <a href="${BUILD_URL}The_20Butler_20report/butler.html">here</a>.<p>
                    --<br>
                    Yours sincerely, The Butler, serving the MySensors community''',
                    mimeType: 'text/html',
                    to: env.CHANGE_AUTHOR_EMAIL,
                    attachLog: true,
                    compressLog: false
                )
            } else {
                slackSend(
                    color: 'danger',
                    message: "Failed to build branch ${env.BRANCH_NAME}. " +
                             "Job <${env.BUILD_URL}|${env.JOB_NAME} #${env.BUILD_NUMBER}> ended with ${result}."
                )
                emailext(
                    subject: "MySensors branch ${env.BRANCH_NAME} failed to build",
                    body: '''I am afraid I failed to build branch ${BRANCH_NAME}. Result was ''' + result + '''.
                    <br>Please check the attached build log or <a href="${BUILD_URL}">here</a> for a hint on what the problem might be.<p>
                    Changes:<br>
                    ${CHANGES}<p>
                    Notable build log lines:<br>
                    ${BUILD_LOG_REGEX, regex="^.*?Terminated.*?$", linesBefore=0, linesAfter=0, maxMatches=5, showTruncatedLines=false, escapeHtml=true}<p>
                    My personal evaluation of your pull request is available <a href="${BUILD_URL}The_20Butler_20report/butler.html">here</a>.<p>
                    --<br>
                    Yours sincerely, The Butler, serving the MySensors community''',
                    mimeType: 'text/html',
                    to: 'builds@mysensors.org',
                    attachLog: true,
                    compressLog: false
                )
            }
        } else {
            config.pr.setBuildStatus(
                config,
                'SUCCESS',
                'Toll gate',
                'Pass',
                ''
            )
        }
    }
}

return this

// Helpers

def resolveGitSha(config, String ref) {
    sh(
        returnStdout: true,
        script: """#!/bin/bash
            cd ${config.repository_root}
            git log -n 1 --pretty=format:'%H' ${ref}
        """
    ).trim()
}

def preRegisterBuildStatuses(config) {
    def baseMsg    = 'Not run yet...'
    def validating = 'Validating...'

    // Context, description, url
    def statuses = [
        [ 'Toll gate',                               validating,              '${BUILD_URL}flowGraphTable/' ],
        [ 'Toll gate (Butler)',                      baseMsg,                 '' ],
        [ 'Toll gate (Code analysis - Cppcheck)',    baseMsg,                 '' ],
        [ 'Toll gate (Documentation)',               baseMsg,                 '' ],
        [ 'Toll gate (Linux builds - Serial GW)',    baseMsg,                 '' ],
        [ 'Toll gate (Linux builds - Ethernet GW)',  baseMsg,                 '' ],
        [ 'Toll gate (Linux builds - MQTT GW)',      baseMsg,                 '' ],
        [ 'Toll gate (Arduino Uno - Tests)',         baseMsg,                 '' ],
		[ 'Toll gate (Arduino Uno - Examples)',      baseMsg,                 '' ],
		[ 'Toll gate (ESP32 - Tests)',               baseMsg,                 '' ],
        [ 'Toll gate (ESP8266 - Tests)',             baseMsg,                 '' ],
        [ 'Toll gate (STM32F1 - Tests)',             baseMsg,                 '' ],
        [ 'Toll gate (STM32F4 - Tests)',             baseMsg,                 '' ],
		[ 'Toll gate (nRF52832 - Tests)',            baseMsg,                 '' ],
        // commented-out entries kept to re-enable later:
        // [ 'Toll gate (MySensorsMicro - Tests)',      baseMsg,                 '' ],
        // [ 'Toll gate (MySensorsGW - Tests)',         baseMsg,                 '' ],
		// [ 'Toll gate (nRF51822 - Tests)',            baseMsg,                 '' ],
        // [ 'Toll gate (nRF5 - Tests)',                baseMsg,                 '' ],
        // [ 'Toll gate (Arduino Mega - Tests)',        baseMsg,                 '' ],
        // [ 'Toll gate (MySensorsMicro - Examples)',   baseMsg,                 '' ],
        // [ 'Toll gate (MySensorsGW - Examples)',      baseMsg,                 '' ],
        // [ 'Toll gate (nRF5 - Examples)',             baseMsg,                 '' ],
        // [ 'Toll gate (STM32F1 - Examples)',          baseMsg,                 '' ],
        // [ 'Toll gate (STM32F4 - Examples)',          baseMsg,                 '' ],
        // [ 'Toll gate (nRF52832 - Examples)',       baseMsg,                 '' ],
        // [ 'Toll gate (nRF51822 - Examples)',       baseMsg,                 '' ],
        // [ 'Toll gate (ESP8266 - Examples)',        baseMsg,                 '' ],
        // [ 'Toll gate (ESP32 - Examples)',          baseMsg,                 '' ],
        // [ 'Toll gate (Arduino Mega - Examples)',   baseMsg,                 '' ],
    ]

    statuses.each { ctx, desc, url ->
        config.pr.setBuildStatus(config, 'PENDING', ctx, desc, url)
    }
}

def checkoutHardware() {
    def repos = [
        [dir: 'hardware/MySensors/avr',  url: 'https://github.com/mysensors/ArduinoHwAVR.git'],
        [dir: 'hardware/MySensors/samd', url: 'https://github.com/mysensors/ArduinoHwSAMD.git'],
        [dir: 'hardware/MySensors/nRF5', url: 'https://github.com/mysensors/ArduinoHwNRF5.git'],
    ]

    repos.each { repo ->
        checkout(
            changelog: false,
            poll:      false,
            scm: [
                $class: 'GitSCM',
                branches: [[name: '*/master']],
                extensions: [
                    [$class: 'CloneOption', depth: 0, noTags: false, reference: '', shallow: true],
                    [$class: 'RelativeTargetDirectory', relativeTargetDir: repo.dir]
                ],
                userRemoteConfigs: [[url: repo.url]]
            ]
        )
    }
}

def runArduinoBuilds(arduino, config) {
    // Tests
    def testTargets = [ 
        [name: 'ESP32 (tests)',          fn: 'buildESP32'],
        [name: 'nRF52832 (tests)',       fn: 'buildnRF52832'],
        [name: 'ESP8266 (tests)',        fn: 'buildESP8266'],
        [name: 'STM32F1 (tests)',        fn: 'buildSTM32F1'],
        [name: 'STM32F4 (tests)',        fn: 'buildSTM32F4'],
        [name: 'ArduinoUno (tests)',     fn: 'buildArduinoUno'],
		// commented-out test builds:
		// [name: 'MySensorsMicro (Tests)', fn: 'buildMySensorsMicro'],
        // [name: 'MySensorsGW (Tests)',    fn: 'buildMySensorsGw'],
        // [name: 'nRF51822 (Tests)',       fn: 'buildnRF51822'],
        // [name: 'nRF5 (Tests)',           fn: 'buildnRF5'],
		// [name: 'ArduinoMega (Tests)',    fn: 'buildArduinoMega'],
    ]
	
	// Examples
    def exampleTargets = [
        [name: 'ArduinoUno (Examples)',        fn: 'buildArduinoUno',     files: 'examples'],
		// commented-out example builds:
		// [name: 'MySensorsMicro (Examples)', fn: 'buildMySensorsMicro', files: 'examples'],
        // [name: 'MySensorsGW (Examples)',    fn: 'buildMySensorsGw',    files: 'examples'],
        // [name: 'nRF5 (Examples)',           fn: 'buildnRF5',           files: 'examples'],
        // [name: 'STM32F1 (Examples)',        fn: 'buildSTM32F1',        files: 'examples'],
        // [name: 'STM32F4 (Examples)',        fn: 'buildSTM32F4',        files: 'examples'],
        // [name: 'nRF52832 (Examples)',       fn: 'buildnRF52832',       files: 'examples'],
        // [name: 'nRF51822 (Examples)',       fn: 'buildnRF51822',       files: 'examples'],
        // [name: 'ESP8266 (Examples)',        fn: 'buildESP8266',        files: 'examples'],
        // [name: 'ESP32 (Examples)',          fn: 'buildESP32',          files: 'examples'],
        // [name: 'ArduinoMega (Examples)',    fn: 'buildArduinoMega',    files: 'examples'],
    ]

    testTargets.each { t ->
        stage(t.name) {
            arduino."${t.fn}"(config, config.tests, 'Tests')
        }
    }

    exampleTargets.each { t ->
        def files = (t.files == 'tests') ? config.tests : config.examples
        stage(t.name) {
            arduino."${t.fn}"(config, files, 'Examples')
        }
    }
}
