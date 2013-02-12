source lib.sh || exit 1

i=./read-order/sanger_s1.fastq.gz
j=./read-order/sanger_s2.fastq.gz
o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)
cmd="$famas -f -i $i -j $j -o $o -p $p -l 20 --quiet"
if ! eval $cmd; then
    echo "Did not complete successfully: $cmd"; exit 1
fi
rm -f $o $p

i=./read-order/sanger_s1_wrong-order.fastq.gz
cmd="$famas -i $i -j $j -o $o -p $p -l 20"
if eval $cmd 2>/dev/null; then
    echo "Should have failed, but didn't: $cmd"; exit 1
fi

rm -f $o $p
cmd="$famas --no-order-check -e 64 -i $i -j $j -o $o -p $p --quiet"
if ! eval $cmd; then
    echo "Did not complete successfully: $cmd"; exit 1
fi
rm -f $o $p

i=./read-order/illumina_s1.fastq.gz
j=./read-order/illumina_s2.fastq.gz
cmd="$famas -e 64 -i $i -j $j -o $o -p $p --quiet"
if ! eval $cmd; then
    echo "Did not complete successfully: $cmd"; exit 1
fi
rm -f $o $p

i=./read-order/illumina_s1_wrong-order.fastq.gz
cmd="$famas -e 64 -i $i -j $j -o $o -p $p --quiet 2>/dev/null"
if eval $cmd; then
    echo "Should have failed, but didn't: $cmd"; exit 1
fi
rm -f $o $p

cmd="$famas --no-order-check -e 64 -i $i -j $j -o $o -p $p --quiet"
if ! eval $cmd; then
    echo "Did not complete successfully: $cmd"; exit 1
fi
rm -f $o $p


exit 0
