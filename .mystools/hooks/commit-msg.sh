#!/bin/bash
too_long_lines=$(awk 'length>72' $1)
if [[ ! -z $too_long_lines ]]; then
	echo "Too long commit message lines:" $too_long_lines
	has_failed=1
fi

leading_lowercases=$(awk 'NR==1' $1 | awk '/^[[:lower:][:punct:]]/')
if [[ ! -z $leading_lowercases ]]; then
	echo "Leading lowercase in subject:" $leading_lowercases
	has_failed=1
fi

trailing_periods=$(awk 'NR==1' $1 | awk '/(\.)$/')
if [[ ! -z $trailing_periods ]]; then
	echo "Trailing periods in subject:" $trailing_periods
	has_failed=1
fi

if [[ ! -z $has_failed ]]; then
	echo "Please recommit with a new commit message."
	exit 1
fi
