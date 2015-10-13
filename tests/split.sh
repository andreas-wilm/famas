#!/bin/bash
#
# test split functionality
#


source lib.sh || exit 1


DEBUG=0
f1=../data/SRR499813_1.Q2-and-N.fastq.gz
f2=../data/SRR499813_2.Q2-and-N.fastq.gz
oext=.fastq.gz


# should fail of XXXXXX not part of template
#
o1=$odir/1.$oext
o2=$odir/2.$oext
cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --split-every 10000 --quiet --no-filter"
if eval $cmd 2>/dev/null; then
    echoerror "The following command should have failed: $cmd"
    exit 1
fi


# should fail if XXXXXX given multiple times
#
o1=$odir/1-XXXXXX-XXXXXX$oext
o2=$odir/2-XXXXXX-XXXXXX$oext
cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --split-every 10000 --quiet --no-filter"
if eval $cmd 2>/dev/null; then
    echoerror "The following command should have failed: $cmd"
    exit 1
fi


# test split is actually happening and that input and split output has
# the same content (no-filter)
#
#
odir=$(mktemp -d -t $0.sh) || exit 1
o1=$odir/1-XXXXXX$oext
o2=$odir/2-XXXXXX$oext
#echodebug "odir=$odir o1=$o1 o2=$o2"
cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --split-every 10000 --quiet --no-filter"
if ! eval $cmd; then
    echoerror "The following command failed: $cmd"
    exit 1
fi

# check number of splits
num_out=$(ls $odir/*$oext | wc -l) 
if [ $num_out -le 2 ]; then
    echoerror "Split unsuccessful: got $num_out files only"
    exit 1
fi


# FIXME check no files has more than split-every reads


# check content
f1md5=$(gzip -dc $f1 | $md5)
f2md5=$(gzip -dc $f2 | $md5)
o1md5=$(gzip -dc $(ls $(echo $o1 | sed -e 's,XXXXXX,*,') | sort) | $md5) || exit 1
o2md5=$(gzip -dc $(ls $(echo $o2 | sed -e 's,XXXXXX,*,') | sort) | $md5) || exit 1
if [ $f1md5 != $o1md5 ]; then
    echoerror "Content changed while splitting first file: compare $f1 and $o1"
    exit 1
fi
if [ $f2md5 != $o2md5 ]; then
    echoerror "Content changed while splitting second file: compare $f1 and $o1"
    exit 1
fi

if [ $DEBUG -eq 1 ]; then
    echodebug "Keeping $odir"
else
    test -d $odir && rm -rf $odir
fi
