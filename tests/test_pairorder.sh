#
# test split functionality
#


source lib.sh || exit 1


DEBUG=0
f1=../data/SRR499813_1.Q2-and-N.fail_order_check.fastq.gz
f2=../data/SRR499813_2.Q2-and-N.fail_order_check.fastq.gz
odir=$(mktemp -d -t $0.XXXXXX.sh) || exit 1
o1=$odir/$(basename $f1)
o2=$odir/$(basename $f2)


cmd="$famas -i $f1 -j $f2 -o $o1 -p $o2 --no-filter --quiet"
if eval $cmd 2>/dev/null; then
    echo "Command should have failed but didn't: $cmd"; exit 1
fi
rm -f $o1 $o2

# should complete without order check
cmd="$cmd --no-order-check"
if ! eval $cmd; then
    echo "Did not complete successfully: $cmd"; exit 1
fi


if [ $DEBUG -ne 1 ]; then
  test -d $odir && rm -rf $odir
else
  echodebug "Leaving $odir"
fi

exit 0
