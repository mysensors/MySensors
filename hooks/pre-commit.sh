#!/bin/sh
if git rev-parse --verify HEAD >/dev/null 2>&1
then
	against=HEAD
else
	# Initial commit: diff against an empty tree object
	against=4b825dc642cb6eb9a060e54bf8d69288fbee4904
fi

OPTIONS="--options=.astylerc -n"
CPPCHECK_OPTIONS="--quiet --error-exitcode=1 --force --enable=style,information --library=avr --platform=tools/cppcheck/avr.xml --includes-file=tools/cppcheck/includes.cfg --suppressions-list=tools/cppcheck/suppressions.cfg"
FILES=$(git diff-index  --cached $against | grep -E '[MA]	.*\.(c|cpp|h|hpp|ino)$' | cut -d'	' -f 2)

RETURN=0
ASTYLE=$(which astyle >/dev/null)
if [ $? -ne 0 ]; then
	echo "astyle not installed or not in current path. Unable to check source file format policy. Make sure you have a clean commit, or the CI system will reject your pull request." >&2
else
	ASTYLE=astyle
	for FILE in $FILES; do
		$ASTYLE $OPTIONS $FILE >/dev/null
		$(git diff-files --quiet $FILE) >&2
		if [ $? -ne 0 ]; then
			echo "[!] $FILE has been updated to match the MySensors core coding style." >&2
			RETURN=1
		fi
	done

	if [ $RETURN -ne 0 ]; then
		echo "" >&2
		echo "Styling updates applied. Review the changes and use 'git add' to update your staged data." >&2
		exit $RETURN
	fi
fi

RETURN=0
CPPCHECK=$(which cppcheck >/dev/null)
if [ $? -ne 0 ]; then
	echo "cppcheck not installed or not in current path. Unable to do static code analysis. Make sure you have a clean commit, or the CI system will reject your pull request." >&2
else
	CPPCHECK=cppcheck
	for FILE in $FILES; do
		$CPPCHECK $CPPCHECK_OPTIONS $FILE 2>&1
		if [ $? -eq 1 ]; then
			echo "[!] $FILE has coding issues." >&2
			RETURN=1
		fi
	done

	if [ $RETURN -ne 0 ]; then
		echo "" >&2
		echo "Make sure you have run cppcheck 1.76.1 or newer cleanly with the following options:" >&2
		echo $CPPCHECK $CPPCHECK_OPTIONS $FILES >&2
		exit 1
	fi
fi
exit $RETURN
