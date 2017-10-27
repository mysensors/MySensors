#!groovy
def call(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Documentation)', 'Generating...', '${BUILD_URL}flowGraphTable/')
	// Generate doxygen file for Raspberry Pi configure command
	sh """#!/bin/bash +x
				cd ${config.repository_root}
				echo -e "/**\n * @defgroup RaspberryPiGateway Raspberry Pi Gateway\n * @ingroup MyConfigGrp\n * @brief Configuration options for the Raspberry Pi Gateway\n@{\n@verbatim" > configure.h
				grep -A999 '<<EOF' configure | grep -B999 EOF | grep -v 'EOF' >> configure.h
				echo -e "@endverbatim\n@}*/\n" >> configure.h"""
	sh """#!/bin/bash +x
				cd ${config.repository_root}
				export PROJECTNUMBER=\$(
					if [[ \$(git rev-parse --abbrev-ref HEAD) == "master" ]]; then
						git describe --tags ;
					else
						git rev-parse --short HEAD ;
					fi
				)
				echo 'WARN_LOGFILE=doxygen.log' >> Doxyfile && doxygen"""
	warnings canComputeNew: false, canResolveRelativePaths: false,
		defaultEncoding: '',
		excludePattern: '''.*/sha204_library\\.h,.*/drivers/Linux/.*,.*/cores/esp8266/.*,hardware/.*''',
		failedTotalAll: '', healthy: '', includePattern: '', messagesPattern: '',
		parserConfigurations: [[parserName: 'Doxygen', pattern: config.repository_root+'doxygen.log']],
		unHealthy: '', unstableTotalAll: '0'
	publishHTML([allowMissing: false, alwaysLinkToLastBuild: false, keepAll: true,
		reportDir: config.repository_root+'Documentation/html',
		reportFiles: 'index.html', reportName: 'Doxygen HTML', reportTitles: ''])

	if (!config.is_pull_request)
	{
		// Publish docs to API server
		if (env.BRANCH_NAME == 'master') {
			sh """#!/bin/bash
						scp -r ${config.repository_root}Documentation/html docs@direct.openhardware.io"""
		} else if (env.BRANCH_NAME == 'development') {
			sh """#!/bin/bash
						scp -r ${config.repository_root}Documentation/html docs@direct.openhardware.io:beta"""
		}
	} else {
		if (currentBuild.currentResult == 'UNSTABLE') {
			config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Documentation)', 'Warnings found', '${BUILD_URL}warnings16Result/new')
			error 'Terminating due to doxygen error'
		} else if (currentBuild.currentResult == 'FAILURE') {
			config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Documentation)', 'Error generating documentation', '${BUILD_URL}flowGraphTable/')
			error 'Terminating due to doxygen error'
		} else {
			config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Documentation)', 'Pass', '${BUILD_URL}Doxygen_HTML/index.html')
		}
	}
}

return this
