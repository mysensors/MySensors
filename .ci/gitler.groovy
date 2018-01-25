#!groovy
def call(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Gitler)', 'Checking...', '${BUILD_URL}flowGraphTable/')
	if (env.CHANGE_TARGET == 'master' &&
		(env.CHANGE_AUTHOR != 'bblacey'     && env.CHANGE_AUTHOR != 'd00616'       &&
		 env.CHANGE_AUTHOR != 'fallberg'    && env.CHANGE_AUTHOR != 'henrikekblad' &&
		 env.CHANGE_AUTHOR != 'marceloaqno' && env.CHANGE_AUTHOR != 'mfalkvidd'    &&
		 env.CHANGE_AUTHOR != 'scalz'       && env.CHANGE_AUTHOR != 'tbowmo'       &&
		 env.CHANGE_AUTHOR != 'tekka007'    && env.CHANGE_AUTHOR != 'user2684'     &&
		 env.CHANGE_AUTHOR != 'Yveaux'))
	{
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Gitler)', 'This pull request targets master. That is not permitted for '+env.CHANGE_AUTHOR, '')
		error "This pull request targets master. That is not permitted!"
	}
	else if (env.CHANGE_TARGET == 'master')
	{
		echo env.CHANGE_AUTHOR + ' is a valid author for targeting master branch, skipping further validation'
		dir(config.repository_root) {
			step([$class: 'GitChangelogRecorder', config: [configFile: 'git-changelog-settings.json',
				createFileTemplateContent: '''
# Changelog
{{#commits}}
### {{{messageTitle}}}
{{{messageBody}}}
[{{hash}}](https://github.com/mysensors/MySensors/commit/{{hash}}) by {{authorName}} at *{{commitTime}}*
{{/commits}}
''',
				createFileTemplateFile: '', createFileUseTemplateContent: true,
				createFileUseTemplateFile: false, customIssues: [[link: '', name: '', pattern: '', title: ''],
				[link: '', name: '', pattern: '', title: '']], dateFormat: 'YYYY-MM-dd HH:mm:ss',
				file: 'ReleaseNotes.md', fromReference: env.CHANGE_TARGET, fromType: 'ref', gitHubApi: '',
				gitHubApiTokenCredentialsId: '', gitHubIssuePattern: '#([0-9]+)', gitHubToken: '',
				gitLabApiTokenCredentialsId: '', gitLabProjectName: '', gitLabServer: '', gitLabToken: '',
				ignoreCommitsIfMessageMatches: '^Merge.*',
				ignoreCommitsWithoutIssue: false, ignoreTagsIfNameMatches: '',
				jiraIssuePattern: '\\b[a-zA-Z]([a-zA-Z]+)-([0-9]+)\\b', jiraPassword: '', jiraServer: '',
				jiraUsername: '', jiraUsernamePasswordCredentialsId: '', mediaWikiPassword: '',
				mediaWikiTemplateContent: '',	mediaWikiTemplateFile: '', mediaWikiTitle: '', mediaWikiUrl: '',
				mediaWikiUseTemplateContent: false, mediaWikiUseTemplateFile: false, mediaWikiUsername: '',
				noIssueName: 'No issue', readableTagName: '/([^/]+?)$', showSummary: false,
				showSummaryTemplateContent: '', showSummaryTemplateFile: '', showSummaryUseTemplateContent: false,
				showSummaryUseTemplateFile: false, subDirectory: '', timeZone: 'UTC',
				toReference: config.git_sha, toType: 'commit', untaggedName: 'Unreleased',
				useConfigFile: false, useFile: true, useGitHub: true, useGitHubApiTokenCredentials: false,
				useGitLab: false, useGitLabApiTokenCredentials: false, useIgnoreTagsIfNameMatches: false,
				useJira: false, useJiraUsernamePasswordCredentialsId: false, useMediaWiki: false,
				useReadableTagName: false, useSubDirectory: false]
			])
		}

		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Gitler)', 'Pass', '')
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Release changelog)', '', '${BUILD_URL}execution/node/3/ws/MySensors/ReleaseNotes.md/*view*/')
		return
	}

	dir(config.repository_root) {
		step([$class: 'GitChangelogRecorder', config: [configFile: 'git-changelog-settings.json',
			createFileTemplateContent: '''
{{#commits}}
{{{messageTitle}}}
{{/commits}}
''',
			createFileTemplateFile: '', createFileUseTemplateContent: true,
			createFileUseTemplateFile: false, customIssues: [[link: '', name: '', pattern: '', title: ''],
			[link: '', name: '', pattern: '', title: '']], dateFormat: 'YYYY-MM-dd HH:mm:ss',
			file: 'subjects.txt', fromReference: env.CHANGE_TARGET, fromType: 'ref', gitHubApi: '',
			gitHubApiTokenCredentialsId: '', gitHubIssuePattern: '#([0-9]+)', gitHubToken: '',
			gitLabApiTokenCredentialsId: '', gitLabProjectName: '', gitLabServer: '', gitLabToken: '',
			ignoreCommitsIfMessageMatches: '^Merge.*',
			ignoreCommitsWithoutIssue: false, ignoreTagsIfNameMatches: '',
			jiraIssuePattern: '\\b[a-zA-Z]([a-zA-Z]+)-([0-9]+)\\b', jiraPassword: '', jiraServer: '',
			jiraUsername: '', jiraUsernamePasswordCredentialsId: '', mediaWikiPassword: '',
			mediaWikiTemplateContent: '',	mediaWikiTemplateFile: '', mediaWikiTitle: '', mediaWikiUrl: '',
			mediaWikiUseTemplateContent: false, mediaWikiUseTemplateFile: false, mediaWikiUsername: '',
			noIssueName: 'No issue', readableTagName: '/([^/]+?)$', showSummary: false,
			showSummaryTemplateContent: '', showSummaryTemplateFile: '', showSummaryUseTemplateContent: false,
			showSummaryUseTemplateFile: false, subDirectory: '', timeZone: 'UTC',
			toReference: config.git_sha, toType: 'commit', untaggedName: 'Unreleased',
			useConfigFile: false, useFile: true, useGitHub: false, useGitHubApiTokenCredentials: false,
			useGitLab: false, useGitLabApiTokenCredentials: false, useIgnoreTagsIfNameMatches: false,
			useJira: false, useJiraUsernamePasswordCredentialsId: false, useMediaWiki: false,
			useReadableTagName: false, useSubDirectory: false]
		])
		step([$class: 'GitChangelogRecorder', config: [configFile: 'git-changelog-settings.json',
			createFileTemplateContent: '''
{{#commits}}
{{#messageBodyItems}}
{{.}} 
{{/messageBodyItems}}
{{/commits}}
''',
			createFileTemplateFile: '', createFileUseTemplateContent: true,
			createFileUseTemplateFile: false, customIssues: [[link: '', name: '', pattern: '', title: ''],
			[link: '', name: '', pattern: '', title: '']], dateFormat: 'YYYY-MM-dd HH:mm:ss',
			file: 'bodies.txt', fromReference: env.CHANGE_TARGET, fromType: 'ref', gitHubApi: '',
			gitHubApiTokenCredentialsId: '', gitHubIssuePattern: '#([0-9]+)', gitHubToken: '',
			gitLabApiTokenCredentialsId: '', gitLabProjectName: '', gitLabServer: '', gitLabToken: '',
			ignoreCommitsIfMessageMatches: '^Merge.*',
			ignoreCommitsWithoutIssue: false, ignoreTagsIfNameMatches: '',
			jiraIssuePattern: '\\b[a-zA-Z]([a-zA-Z]+)-([0-9]+)\\b', jiraPassword: '', jiraServer: '',
			jiraUsername: '', jiraUsernamePasswordCredentialsId: '', mediaWikiPassword: '',
			mediaWikiTemplateContent: '',	mediaWikiTemplateFile: '', mediaWikiTitle: '', mediaWikiUrl: '',
			mediaWikiUseTemplateContent: false, mediaWikiUseTemplateFile: false, mediaWikiUsername: '',
			noIssueName: 'No issue', readableTagName: '/([^/]+?)$', showSummary: false,
			showSummaryTemplateContent: '', showSummaryTemplateFile: '', showSummaryUseTemplateContent: false,
			showSummaryUseTemplateFile: false, subDirectory: '', timeZone: 'UTC',
			toReference: config.git_sha, toType: 'commit', untaggedName: 'Unreleased',
			useConfigFile: false, useFile: true, useGitHub: false, useGitHubApiTokenCredentials: false,
			useGitLab: false, useGitLabApiTokenCredentials: false, useIgnoreTagsIfNameMatches: false,
			useJira: false, useJiraUsernamePasswordCredentialsId: false, useMediaWiki: false,
			useReadableTagName: false, useSubDirectory: false]
		])
	}

	ret = sh(returnStatus: true,
		script:"""#!/bin/bash
							cd ${config.repository_root}/.ci
							./gitler.sh""")

	if (fileExists(config.repository_root+'restyling.patch')) {
		emailext (
			subject: "Job '${env.JOB_NAME} #${env.BUILD_NUMBER} [PR#${env.CHANGE_ID}]' failed due to bad code styling",
			body: """<p>Job '${env.JOB_NAME} [<a href="${env.CHANGE_URL}">PR#${env.CHANGE_ID}</a> - ${env.CHANGE_TITLE}]' failed because code style does not follow the standards.</p>
			A patch to rectify the errors is attached. You apply the patch using:<br>
			git apply restyling.patch<p>
			If you disagree to this, please discuss it <a href="${env.CHANGE_URL}">here</a>.<p>
			Yours sincerely, Gitler, on behalf of Jenkins""",
			mimeType: 'text/html', to: '${env.CHANGE_AUTHOR_EMAIL}',
			attachLog: false, compressLog: false, attachmentsPattern: config.repository_root+'restyling.patch'
		)
	}
	publishHTML([allowMissing: true, alwaysLinkToLastBuild: false, keepAll: true,
		reportDir: config.repository_root,
		reportFiles: 'gitler.html', reportName: 'Gitler report', reportTitles: ''])
	if (ret == 1) {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Gitler)', 'Commit(s) does not meet coding standards', '${BUILD_URL}Gitler_report/gitler.html')
		currentBuild.currentResult == 'FAILURE'
		echo "Termiated due to Gitler assert" // For BFA
		echo "You can read the detailed error report here: "+env.BUILD_URL+"Gitler_report/"
		error 'Termiated due to Gitler assert'
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Gitler)', 'Pass', '')
	}
}

return this
