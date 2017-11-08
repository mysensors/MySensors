#!/bin/bash

# Instantiate the tool runtime shim
. "$(git mystoolspath).bundle_runtime.sh"

# cppcheck takes one or more files as an argument.
# --cached or <no arg> for changed
if [[ $# = 0 ]]; then
	for file in $(modifiedSourceFiles); do
		if [[ $file != *.h ]]; then
			runBundle $file
		fi
	done
elif [[ $1 = '--cached' ]]; then
	for file in $(stagedSourceFiles); do
		if [[ $file != *.h ]]; then
			runBundle $file
		fi
	done
else
	eval "set -- $(git rev-parse --sq --prefix "$GIT_PREFIX" "$@")"
	runBundle "$@"
fi
