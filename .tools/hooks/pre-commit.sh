#!/bin/bash

# instantiate mysensors tool runtime shim
. "$(git mystoolspath).bundle_runtime.sh"

###
#  astyle 
#
errorsDetected=0
for file in $(stagedSourceFiles); do
	git astyle $file >/dev/null
	if ! git diff-files --quiet $file >&2; then
		warn "$file has been updated to match the MySensors core coding style."
		errorsDetected=1
	fi
done

[ $errorsDetected == 1 ] && err "Styling updates applied. Review the changes and use 'git add' to update your staged files."

###
#  cppcheck
#
errorsDetected=0
for file in $(stagedSourceFiles); do
	if ! git cppcheck $file >/dev/null; then
		warn "$file failed static analysis."
		errorsDetected=1
	fi
done

[ $errorsDetected == 1 ] && err "Correct the errors until cppcheck passees using 'git cppcheck --cached'."

exit 0
