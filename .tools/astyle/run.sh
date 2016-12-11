#!/bin/bash

# Instantiate the bundle runtime shim
. "$(git mystoolspath).bundle_runtime.sh"

# Astyle takes one or more files as an argument.
# --cached or <no arg> for changed
if [[ $# = 0 ]]; then
	for file in $(modifiedSourceFiles); do
		runBundle $file
	done
elif [[ $1 = '--cached' ]]; then
	for file in $(stagedSourceFiles); do
		runBundle $file
	done
else
	eval "set -- $(git rev-parse --sq --prefix "$GIT_PREFIX" "$@")"
	runBundle "$@"
fi
