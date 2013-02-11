famas=../../src/famas
i=./one_too_many/one_seq.fastq.gz
j=./one_too_many/two_seqs.fastq.gz
o=./one_too_many/tmp_out_1.gz
p=./one_too_many/tmp_out_2.gz
$famas -f -i $i -j $j -o $o -p $p >>log.txt 2>&1 && exit 1
$famas -f -j $j -i $i -o $o -p $p >>log.txt 2>&1 && exit 1
exit 0
