#!groovy
def setBuildStatus(config, String state, String context, String message, String backref) {
	if (config.is_pull_request) {
		step([$class: 'GitHubCommitStatusSetter',
			reposSource: [$class: 'ManuallyEnteredRepositorySource', url: "https://github.com/${config.github_organization}/${config.repository_name}"],
			errorHandlers: [[$class: 'ShallowAnyErrorHandler']],
			contextSource: [$class: 'ManuallyEnteredCommitContextSource', context: context],
			commitShaSource: [$class: "ManuallyEnteredShaSource", sha: config.git_sha],
			statusBackrefSource: [$class: 'ManuallyEnteredBackrefSource', backref: backref],
			statusResultSource: [$class: 'ConditionalStatusResultSource',
			results: [[$class: 'AnyBuildResult', message: message, state: state]]]]
		)
	}
}

return this