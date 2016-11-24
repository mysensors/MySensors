#!/bin/sh

OPTIONS="--options=.astylerc"

RETURN=0
ASTYLE=astyle
if [ $? -ne 0 ]; then
	echo "[!] astyle not installed or not in current path. Unable to check source file format policy." >&2
	exit 1
fi

FILES=`git diff --name-only --diff-filter=ACMR | grep -E "\.(c|cpp|h|ino)$"`
for FILE in $FILES; do
	$ASTYLE $OPTIONS < $FILE | cmp -s $FILE -
	if [ $? -ne 0 ]; then
		echo "[!] $FILE does not respect the agreed coding style." >&2
		RETURN=1
	fi
done

if [ $RETURN -eq 1 ]; then
	echo "" >&2
	echo "Make sure you have run astyle with the following options:" >&2
	echo $OPTIONS >&2
fi

exit $RETURN