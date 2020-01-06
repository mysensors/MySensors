#!groovy
def call(config) {
	config.pr.setBuildStatus(config, 'PENDING', 'Toll gate (Butler)', 'Checking...', '${BUILD_URL}flowGraphTable/')
	if (env.CHANGE_TARGET == 'master' &&
		(env.CHANGE_AUTHOR != 'bblacey'     && env.CHANGE_AUTHOR != 'd00616'       &&
		 env.CHANGE_AUTHOR != 'fallberg'    && env.CHANGE_AUTHOR != 'henrikekblad' &&
		 env.CHANGE_AUTHOR != 'marceloaqno' && env.CHANGE_AUTHOR != 'mfalkvidd'    &&
		 env.CHANGE_AUTHOR != 'scalz'       && env.CHANGE_AUTHOR != 'tbowmo'       &&
		 env.CHANGE_AUTHOR != 'tekka007'    && env.CHANGE_AUTHOR != 'user2684'     &&
		 env.CHANGE_AUTHOR != 'Yveaux'))
	{
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Butler)', 'This pull request targets master. I am afraid that is not permitted for '+env.CHANGE_AUTHOR, '')
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

		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Butler)', 'Pass - Well done!', '')
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
							./butler.sh""")

	if (fileExists(config.repository_root+'restyling.patch')) {
		emailext (
			subject: "PR#${env.CHANGE_ID} - ${env.CHANGE_TITLE} has unfortunate code styling",
			body: """<p>Greetings!<p>
			I am afraid your pull request does not follow the MySensors standards with respect to coding style.</p>
			That is ok, you are perhaps a first time committer to this repository. Please read the <a href="https://www.mysensors.org/download/contributing">code contribution guidelines</a> for help on how to format your code.<p>
			To assist you, I have prepared a patch for you that will reformat the code according to the coding style required.<br>
			The patch is attached. You may apply the patch using:<br>
			git apply restyling.patch<p>
			If you disagree with me, please discuss it <a href="${env.CHANGE_URL}">here</a>.<p>
			--<br>
			Yours sincerely, The Butler, serving the MySensors community""",
			mimeType: 'text/html', to: env.CHANGE_AUTHOR_EMAIL,
			attachLog: false, compressLog: false, attachmentsPattern: config.repository_root+'restyling.patch'
		)
	}
	publishHTML([allowMissing: true, alwaysLinkToLastBuild: false, keepAll: true,
		reportDir: config.repository_root,
		reportFiles: 'butler.html', reportName: 'The Butler report', reportTitles: ''])
	if (ret == 1) {
		config.pr.setBuildStatus(config, 'FAILURE', 'Toll gate (Butler)', 'I am afraid the commit(s) needs some touchup, please check the details...', '${BUILD_URL}The_20Butler_20report/butler.html')
		currentBuild.currentResult == 'FAILURE'
		echo "Terminated due to Butler assert" // For BFA
		echo "You can read the detailed error report here: "+env.BUILD_URL+"The_20Butler_20report/"
		error 'Terminated due to Butler assert'
	} else {
		config.pr.setBuildStatus(config, 'SUCCESS', 'Toll gate (Butler)', 'Pass - Well done!', '${BUILD_URL}The_20Butler_20report/butler.html')
	}
}

return this
