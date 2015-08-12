source lib.sh || exit 1

i=./read-order/sanger_s1.fastq.gz
j=./read-order/sanger_s2.fastq.gz
o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)
rm -f $o $p

gzip -dc $i | $famas -i - -j $j -o $o -p $p -l 20 --quiet || exit 1
rm -f $o $p

cat $i | $famas -i - -j $j -o $o -p $p -l 20 --quiet || exit 1
rm -f $o $p

$famas -i $i -j $j -o - -p $p -l 20 --quiet | gzip >/dev/null|| exit 1
rm -f $o $p
