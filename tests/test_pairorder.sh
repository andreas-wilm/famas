famas=../src/famas

ctr=1

i=./read-order/sanger_s1.fastq.gz
j=./read-order/sanger_s2.fastq.gz
o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)
#let ctr=ctr+1; echo "DEBUG test $ctr"
$famas -f -c -i $i -j $j -o $o -p $p -l 20 --quiet || exit 1
rm -f $o $p

i=./read-order/sanger_s1_wrong-order.fastq.gz
#let ctr=ctr+1; echo "DEBUG test $ctr"
$famas -c -i $i -j $j -o $o -p $p -l 20 2>/dev/null  && exit 1
rm -f $o $p

i=./read-order/illumina_s1.fastq.gz
j=./read-order/illumina_s2.fastq.gz
#let ctr=ctr+1; echo "DEBUG test $ctr"
$famas -c -e 64 -i $i -j $j -o $o -p $p --quiet || exit 1
rm -f $o $p

i=./read-order/illumina_s1_wrong-order.fastq.gz
#let ctr=ctr+1; echo "DEBUG test $ctr"
$famas -c -e 64 -i $i -j $j -o $o -p $p --quiet 2>/dev/null && exit 1
rm -f $o $p


exit 0
