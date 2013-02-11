md5=$(which md5sum 2>/dev/null || which md5)
famas=../src/famas
i=./fastq-illumina/GA005-PE-R00071_ACAGTG_s_3_1.fastq.gz
j=./fastq-illumina/GA005-PE-R00071_ACAGTG_s_3_2.fastq.gz
o=./fastq-illumina/tmp_out_s1.gz
p=./fastq-illumina/tmp_out_s2.gz
$famas --quiet -f -Q 0 -q 3 -l 60 -e 64 -i $i -j $j -o $o -p $p || exit 1
md5orig=$(zcat ${i%.fastq.gz}_3pq3l60.fastq.gz ${j%.fastq.gz}_3pq3l60.fastq.gz | $md5)
md5new=$(zcat $o $p | $md5)
test $md5orig == $md5new || exit 1
exit 0
