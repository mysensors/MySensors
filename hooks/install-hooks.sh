#!/bin/bash
PRECOMMIT=hooks/pre-commit
GITROOT=$(git rev-parse --git-dir)
if [[ "$OSTYPE" == "linux-gnu" ]]; then
	rm -f $GITROOT/$PRECOMMIT
	ln -s ../../$PRECOMMIT.sh $GITROOT/$PRECOMMIT
elif [[ "$OSTYPE" == "darwin"* ]]; then
	rm -f $GITROOT/$PRECOMMIT
	ln -s ../../$PRECOMMIT.sh $GITROOT/$PRECOMMIT
elif [[ "$OSTYPE" == "cygwin" ]]; then
	rm -f $GITROOT/$PRECOMMIT
	cp $GITROOT/../$PRECOMMIT.sh $GITROOT/$PRECOMMIT
elif [[ "$OSTYPE" == "msys" ]]; then
	rm -f $GITROOT/$PRECOMMIT
	cp $GITROOT/../$PRECOMMIT.sh $GITROOT/$PRECOMMIT
elif [[ "$OSTYPE" == "freebsd"* ]]; then
	rm -f $GITROOT/$PRECOMMIT
	ln -s ../../$PRECOMMIT.sh $GITROOT/$PRECOMMIT
else
	echo "Unknown/unsupported OS, won't install anything"
fi
