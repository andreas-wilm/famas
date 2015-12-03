#
# test pair order chechk functionality
#


source lib.sh || exit 1


DEBUG=0
f1=../data/SRR499813_1.Q2-and-N.fail_number_check.fastq.gz
f2=../data/SRR499813_2.Q2-and-N.fail_number_check.fastq.gz
odir=$(mktemp -d -t $0..sh.XXX) || exit 1
o1=$odir/$(basename $f1)
o2=$odir/$(basename $f2)


# number mismatch doesn't matter in single end mode

cmd="$famas -i $f1 -o $o1 --quiet"
if ! eval $cmd 2>/dev/null; then
    echo "The following command failed: $cmd"; exit 1
fi
rm -f $o1

cmd="$famas -i $f2 -o $o2 --quiet"
if ! eval $cmd 2>/dev/null; then
    echo "The following command failed: $cmd"; exit 1
fi
rm -f $o2


cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --quiet"
if eval $cmd 2>/dev/null; then
    echo "The following command should have failed: $cmd"; exit 1
fi


if [ $DEBUG -ne 1 ]; then
    test -d $odir && rm -rf $odir
else
    echodebug "Leaving $odir"
fi

exit 0
