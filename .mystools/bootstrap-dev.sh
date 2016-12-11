#!/bin/bash
###
#  OVERVIEW:
#			This script bootstraps the MySensors development environment.
#
#  USAGE:
#			git clone https://github.com/mysensors/MySensors.git
#     cd MySensors
#			.mystools/bootstrap-dev.sh
##

##
# Import common utility functions and environment variables
. "$(cd "$(dirname "${BASH_SOURCE[0]}")"; pwd)/.bundle_runtime.sh"

check_tool_prerequisites()
{
	local err=0
	for preReq in ${1}; do
		if ! is_installed $preReq; then
			warn "$preReq not installed or not in current path."
			err=1
		fi
	done
	return $err
}

check_git_remote()
{
	local url=$( git config --get remote.${1}.url )
	[[ $url == ${2} ]]
}

install_hooks()
{
	for hook in "${1}"/*.sh; do
		local hookname=$(basename $hook)
		$(cd "${GITDIR}/hooks"; ln -s -f "../../${TOOLSDIR}hooks/${hookname}" "${hookname%.sh}")
	done
}

###
#  Main entry
#
#  1.  Check that we are bootstrapping a supported OS/environment
#  2.  Validate github remotes include valid upstream and origin
#  3.  Check for client commit hook prerequisites
#  4.  Install client commit hook prerequisites
#  5.  Define aliases for conveniently running tool bundles
##

mysrepo="https://github.com/mysensors/MySensors.git" 

#1
log "Checking operating system support: ${OSTYPE}..."
is_supported_os ${OSTYPE} || {
	err "OS ${OSTYPE} is unknown/unsupported, won't install anything"
}

#2
log "Checking github 'origin' & 'upstream' remotes..."
check_git_remote "origin" "${mysrepo}" && warn "Git \"origin\" should point to your github fork, not the github MySensors repo ${mysrep}"

check_git_remote "upstream" "${mysrepo}" || {
	warn "Git \"upstream\" remote not found or incorrectly defined. Configuring remote upstream --> ${mysrepo}..."
	git remote add upstream "${mysrepo}" || err "git remote add ${mysrep} failed due to error $?"
}

#3
log "Checking tool/utility prerequisites..."
check_tool_prerequisites "astyle cppcheck" || err "One or more required tools not found.  Install required tools and re-run ${0}"

#4
log "Installing client-side git hooks..."
install_hooks "$(git mystoolspath)hooks" || err "Failed to install git hooks due to error $?..."

#5
log "Configuring git aliases for running mysensor tools..."
configure_git_tool_aliases || err "Failed to create git aliases due to error $?..."

bootstrap_version "--set"

log "Successfully configured your repo for MySensors development... Thanks for your support!"
