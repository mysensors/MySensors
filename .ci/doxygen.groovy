#!groovy
def call(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Documentation)', 'Generating...', '${BUILD_URL}flowGraphTable/')
	sh """#!/bin/bash
				cd ${config.repository_root}
				Documentation/doxygen.sh"""
	warnings canComputeNew: false, canResolveRelativePaths: false,
		defaultEncoding: '',
		excludePattern: '''.*/hal/architecture/Linux/drivers/.*,.*/drivers/TinyGSM/.*''',
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
						scp -r ${config.repository_root}Documentation/html docs@direct.openhardware.io:"""
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
