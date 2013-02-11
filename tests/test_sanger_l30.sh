md5=$(which md5sum 2>/dev/null || which md5)
famas=../src/famas
i=./fastq-sanger/mux079-pdm003_s1.fastq.gz
j=./fastq-sanger/mux079-pdm003_s2.fastq.gz
o=./fastq-sanger/tmp_out_s1.gz
p=./fastq-sanger/tmp_out_s2.gz
#cat <<EOF
$famas --quiet -f -Q 0 -q 3 -l 30 -i $i -j $j -o $o -p $p || exit 1
#EOF
md5orig=$(zcat ${i%.fastq.gz}_3pq3l60.fastq.gz ${j%.fastq.gz}_3pq3l60.fastq.gz | $md5)
md5new=$(zcat $o $p | $md5)
test $md5orig == $md5new || exit 1
exit 0
