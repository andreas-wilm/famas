#!/bin/bash

#
# test for memory leaks in complex call (PE split)
#


source lib.sh || exit 1


DEBUG=0
f1=../data/SRR499813_1.Q2-and-N.fastq.gz
f2=../data/SRR499813_2.Q2-and-N.fastq.gz
oext=.fastq.gz


odir=$(mktemp -d -t $0.sh) || exit 1
o1=$odir/R1-XXXXXX.$oext
o2=$odir/R2-XXXXXX.$oext
valgrind_log=$odir/valgrind.log


# as in split.sh but with filter and sampling
cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --split-every 1000 --sampling 10 --quiet"
dsym_arg="--dsymutil=yes "
#dsym_arg=""
valgrind --log-file=$valgrind_log --track-origins=yes --leak-check=full --tool=memcheck $dsym_arg --show-leak-kinds=all  $cmd || exit 1
 

num_err=$(grep 'ERROR SUMMARY' $valgrind_log | grep -cv ': 0 errors')
if [ "$num_err" -ne 0 ]; then
    echoerror "Found errors in Valgrind output $valgrind_log"
    exit 1
fi


lost_bytes=$(grep 'lost' $valgrind_log | grep -cv ': 0 bytes in 0 blocks')
if [ "$lost_bytes" -ne 0 ]; then
    echoerror "Found lost bytes in Valgrind output $valgrind_log" || exit 1
    exit 1
fi


if [ $DEBUG -eq 1 ]; then
    echodebug "Keeping $odir"
else
    test -d $odir && rm -rf $odir
fi
