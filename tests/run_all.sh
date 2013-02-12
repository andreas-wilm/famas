#!/bin/bash

rm -f log.txt
for f in $(ls test*sh); do
	echo -n "Running $(basename $f): " | tee -a log.txt
	if bash $f >> log.txt 2>&1; then
		echo "OK"  | tee -a log.txt;
	else
		echo "FAILED. See log.txt" | tee -a log.txt;
	fi
done
