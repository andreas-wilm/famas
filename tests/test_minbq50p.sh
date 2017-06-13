#!/bin/bash
#
# test split functionality
#


source lib.sh || exit 1


DEBUG=0
i=../data/minbq50p_1.fastq.gz
j=../data/minbq50p_2.fastq.gz
odir=$(mktemp -d -t $0..sh.XXX) || exit 1
o=$odir/o1.fastq.gz
p=$odir/o2.fastq.gz


cmd="$famas -i $i -j $j -o $o -p $p --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
num_out=$(zgrep -c '^@R' $o)
if [ $num_out -ne 6 ]; then
    echoerror "Expected 6 but got $num_out sequences from $cmd"
    exit 1
fi
rm $o $p

cmd="$famas -i $i -j $j -o $o -p $p -m 2 --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
num_out=$(zgrep -c '^@R' $o)
if [ $num_out -ne 4 ]; then
    echoerror "Expected 4 but got $num_out sequences from $cmd"
    exit 1
fi
rm $o $p

cmd="$famas -i $j -j $i -o $p -p $o -m 2 --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
num_out=$(zgrep -c '^@R' $o)
if [ $num_out -ne 4 ]; then
    echoerror "Expected 4 but got $num_out sequences from $cmd"
    exit 1
fi
rm $o $p


cmd="$famas -i $i -j $j -o $o -p $p -m 9 --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
num_out=$(zgrep -c '^@R' $o)
if [ $num_out -ne 4 ]; then
    echoerror "Expected 4 but got $num_out sequences from $cmd"
    exit 1
fi
rm $o $p


cmd="$famas -i $i -j $j -o $o -p $p -m 10 --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
num_out=$(zgrep -c '^@R' $o)
if [ $num_out -ne 0 ]; then
    echoerror "Expected 0 but got $num_out sequences from $cmd"
    exit 1
fi
rm $o $p


if [ $DEBUG -eq 1 ]; then
    echodebug "Keeping $odir"
else
    test -d $odir && rm -rf $odir
fi
