#!groovy
def cppCheck(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Code analysis - Cppcheck)', 'Running...', '${BUILD_URL}flowGraphTable/')
	// We could consider running Cppcheck for GNU as well, but to avoid so many duplicates, we stick to AVR
	sh """#!/bin/bash +x
				cd ${config.repository_root}
				echo "Doing cppcheck for AVR..."
				find . -type f \\( -iname \\*.c -o -iname \\*.cpp -o -iname \\*.ino \\) | cppcheck -j 4 --force --file-list=- --enable=style,information --platform=.mystools/cppcheck/config/avr.xml --suppressions-list=.mystools/cppcheck/config/suppressions.cfg --includes-file=.mystools/cppcheck/config/includes.cfg --language=c++ --inline-suppr --xml --xml-version=2 2> cppcheck-avr.xml
				cppcheck-htmlreport --file="cppcheck-avr.xml" --title="cppcheck-avr" --report-dir=cppcheck-avr_cppcheck_reports --source-dir=."""
	
	publishHTML([allowMissing: false, alwaysLinkToLastBuild: false, keepAll: true,
		reportDir: config.repository_root+'cppcheck-avr_cppcheck_reports',
		reportFiles: 'index.html', reportName: 'CppCheck AVR', reportTitles: ''])

	step([$class: 'ViolationsToGitHubRecorder', 
		config: [
			repositoryName: config.repository_name, 
			pullRequestId: env.CHANGE_ID, 
			createCommentWithAllSingleFileComments: true, 
			createSingleFileComments: true, 
			commentOnlyChangedContent: true,
			keepOldComments: false,
			violationConfigs: [[pattern: '.*/cppcheck-avr\\.xml$', parser: 'CPPCHECK', reporter: 'Cppcheck'],]
		]
	])
	ret = sh(returnStatus: true,
		script: "#!/bin/bash +e\n"+
				"cd ${config.repository_root}\n"+
				"grep -q \"<td>0</td><td>total</td>\" cppcheck-avr_cppcheck_reports/index.html || exit_code=\$?\n"+
				"exit \$((exit_code == 0 ? 0 : 1))")
	if (ret == 1) {
		config.pr.setBuildStatus(config, 'ERROR', 'Toll gate (Code analysis - Cppcheck)', 'Issues found', '${BUILD_URL}CppCheck_20AVR/index.html')
		error 'Terminating due to Cppcheck error'
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Code analysis - Cppcheck)', 'Pass', '')
	}
}

return this