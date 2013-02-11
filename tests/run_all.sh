#!/bin/bash

for f in $(ls test*sh); do
	echo -n "Running $(basename $f): "
	if bash $f; then
		echo "OK";
	else
		echo "FAILED";
	fi
done