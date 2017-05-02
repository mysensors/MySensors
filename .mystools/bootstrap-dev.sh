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

check_tool_prerequisite()
{
	function ver { printf "%03d%03d%03d%03d" $(echo "$1" | tr '.' ' '); }

	if is_installed ${1} ; then
		#local version=$(${1} --version 2>&1 | sed -e 's/[[:alpha:]|(|[:space:]]//g')
		local version=$(${1} --version 2>&1 | sed -ne 's/[^0-9]*\(\([0-9]\.\)\{0,4\}[0-9][^.]\).*/\1/p')
		if [ $(ver ${version}) -lt $(ver ${2}) ]; then
			warn "Found ${1} ${version} however, version ${2} or greater is required..."
			return 1
		fi
	else
		warn "${1} not installed or not in current path."
		return 1
	fi
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
check_tool_prerequisite "astyle" "2.0.5"   || err "Install AStyle 2.0.5 or greater and re-run ${0}"
check_tool_prerequisite "cppcheck" "1.76" || err "Install Cppcheck 1.76 or greater and re-run ${0}"
check_tool_prerequisite "git" "2.0" || err "Install git 2.0 or greater and re-run ${0}"

#4
log "Installing client-side git hooks..."
install_hooks "$(git mystoolspath)hooks" || err "Failed to install git hooks due to error $?..."

#5
log "Configuring git aliases for running mysensor tools..."
configure_git_tool_aliases || err "Failed to create git aliases due to error $?..."

bootstrap_version "--set"

info "Successfully configured your repo for MySensors development... Thanks for your support!"
