famas=../../src/famas
i=./one_too_many/one_seq.fastq.gz
j=./one_too_many/two_seqs.fastq.gz
o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)
$famas -f -i $i -j $j -o $o -p $p 2>/dev/null && exit 1
$famas -f -j $j -i $i -o $o -p $p 2>/dev/null && exit 1
exit 0
