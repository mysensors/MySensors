#!/bin/bash
if [ ! -d mysensors-release ]; then
	git clone -o upstream git@github.com:mysensors/MySensors.git mysensors-release
fi
cd mysensors-release || exit
git fetch upstream
git checkout remotes/upstream/development
git checkout -b tmp
git merge -s ours remotes/upstream/master
git checkout remotes/upstream/master
git checkout -b release
git merge tmp
git branch -D tmp
git diff remotes/upstream/development
