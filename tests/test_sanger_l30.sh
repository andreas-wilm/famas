source lib.sh || exit 1

i=./fastq-sanger/mux079-pdm003_s1.fastq.gz
j=./fastq-sanger/mux079-pdm003_s2.fastq.gz

o=$(mktemp -t $(basename $0).XXXXXX)
p=$(mktemp -t $(basename $0).XXXXXX)

#echodebug "famas=$famas"
#cat <<EOF
$famas --quiet -f -Q 0 -q 3 -l 30 -i $i -j $j -o $o -p $p || exit 1
#EOF
md5orig=$(gzip -dc ${i%.fastq.gz}_3pq3l60.fastq.gz ${j%.fastq.gz}_3pq3l60.fastq.gz | $md5 | cut -f 1 -d ' ')
md5new=$(gzip -dc $o $p | $md5 | cut -f 1 -d ' ')
test $md5orig == $md5new || exit 1

exit 0
