#!/bin/bash
###
#
#  Tool runtime shim that provides a simple API for running
#  tool bundles and convenience functions for other tool developers.
#
##
log()  { IFS=" "; >&2 printf "%s\n" "$*";										}
info() { IFS=" "; >&2 printf "\e[92m%s\e[m\n" "$*";					}
warn() { IFS=" "; >&2 printf "\e[93m%s\e[m\n" "$*";					}
err()  { IFS=" "; >&2 printf "\e[91m%s\e[m\n" "$*"; exit 1;	}

is_supported_os()
{
	[[ ${1} == darwin* ]] || [[ ${1} == linux-gnu* ]] || [[ ${1} == freebsd ]] || [[ ${1} == msys ]] || [[ ${1} == cygwin ]]
}

is_installed()
{
	which "${1}" > /dev/null 2>&1
}

tools_dir()
{
	git_config_tools_dir='git config mysensors.toolsdir'

	# Set mysensors.toolsdir in git config if it hasn't been set
	if [[ ! $($git_config_tools_dir) || ! -d "$($git_config_tools_dir)" ]]; then
		$git_config_tools_dir "$( cd "$(dirname "${BASH_SOURCE[0]}")"; git rev-parse --show-prefix )"
		git config alias.mystoolspath '![ -z "${GIT_PREFIX}" ] || cd ${GIT_PREFIX}; echo $(git rev-parse --show-cdup)$(git config mysensors.toolsdir)'
	fi

  $git_config_tools_dir	
}

configure_git_tool_aliases()
{
	find "$(git mystoolspath)" -name run.sh -print0 | while IFS= read -r -d '' bundle; do
		local tool="$(basename "$(dirname "${bundle}")")"
		git config alias.${tool} '!f() { $(git mystoolspath)'${tool}'/run.sh $@; }; f'
	done
}

bootstrap_cksum()
{
	echo $(git ls-files -s -- $(git mystoolspath)bootstrap-dev.sh | cut -d' ' -f2)
}

bootstrap_version()
{
	git_config_bootstrap_version='git config mysensors.bootstrap-cksum'
	if [[ $1 == --set ]]; then
		$git_config_bootstrap_version $(bootstrap_cksum)
	else
		echo $($git_config_bootstrap_version)
	fi
}

environment_outdated()
{
	[[ $(bootstrap_version) != $(bootstrap_cksum) ]]
}

modifiedSourceFiles()
{
	against=$(git rev-parse --verify HEAD >/dev/null 2>&1 && echo HEAD || echo 4b825dc642cb6eb9a060e54bf8d69288fbee4904)
	git diff $1 --diff-filter=AM --name-only $against | grep -E '.*\.(c|cpp|h|hpp|ino)$'
}

stagedSourceFiles()
{
	modifiedSourceFiles '--cached'
}

runBundle()
{
	local bundle="$(dirname "${0}")"
	local tool=$(basename "${bundle}")

	local  CMD_OPTIONS=$( TOOLCONFIG="${bundle}/config"	"${bundle}/options.sh" )

	$tool $CMD_OPTIONS "$@"
}

###
#  Common environment variables
#
$(git rev-parse --is-inside-work-tree --quiet >/dev/null 2>&1) || err "Working directory is not a git repository.  aborting..."

if [[ $(basename "${0}") != *bootstrap* ]] && environment_outdated ; then
	err "Your environment is out of date...  Re-run $(git mystoolspath)bootstrap-dev.sh and try again"
fi

GITREPO=$(git rev-parse --show-toplevel)
GITDIR=$(git rev-parse --git-dir)
TOOLSDIR="$(tools_dir)"
