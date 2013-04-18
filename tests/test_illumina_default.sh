source lib.sh || exit 1

md5=$(which md5sum 2>/dev/null || which md5)
famas=../src/famas
i=./fastq-illumina/GA005-PE-R00071_ACAGTG_s_3_1.fastq.gz
j=./fastq-illumina/GA005-PE-R00071_ACAGTG_s_3_2.fastq.gz
o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)
cmd="$famas --quiet -f -Q 0 -q 3 -l 60 -e 64 -i $i -j $j -o $o -p $p"
if ! eval $cmd; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
md5orig=$(zcat ${i%.fastq.gz}_3pq3l60.fastq.gz ${j%.fastq.gz}_3pq3l60.fastq.gz | $md5 | cut -f 1 -d ' ')
md5new=$(zcat $o $p | $md5 | cut -f 1 -d ' ')
#echodebug "md5orig=$md5orig md5new=$md5new"
test $md5orig == $md5new || exit 1
rm -f $o $p
exit 0
