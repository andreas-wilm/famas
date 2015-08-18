source lib.sh || exit 1

i=./fastq-sanger/mux079-pdm003_s1.fastq.gz
j=./fastq-sanger/mux079-pdm003_s2.fastq.gz

#md5orig=$(gzip -dc $i | $md5 | cut -f 1 -d ' ')

#cat <<EOF
o=/tmp/s1_n.gz
p=/tmp/s2_n.gz
rm -f $o $p 2>/dev/null

cmd="$famas --quiet --no-filter -i $i -j $j -o $o -p $p"
#echo "DEBUG $cmd"
eval $cmd || exit 1

# shouldn't be allowed to overwrite
eval $cmd 2>/dev/null && exit 1

# append
cmd="$cmd -a"
#echo "DEBUG $cmd"
eval $cmd || exit 1
 

# added same files twice. output should be twice the size
ni=$(gzip -dc $i | wc -l)
nj=$(gzip -dc $j | wc -l)
no=$(gzip -dc $o | wc -l)
np=$(gzip -dc $p | wc -l)
test $ni -eq $nj || echoerror "number of input reads differ"
test $no -eq $np || echoerror "number of output reads differ"
test $no -eq $(( $ni*2 )) || echoerror "number of output reads differ"
    
# added same files twice. can't contain unique lines
ni_u=$(gzip -dc $i | sort -u | wc -l)
nj_u=$(gzip -dc $j | sort -u | wc -l)
no_u=$(gzip -dc $o | sort -u | wc -l)
np_u=$(gzip -dc $p | sort -u | wc -l)
test $ni_u -eq $nj_u || echoerror "uniq lines differ"
test $ni_u -eq $no_u || echoerror "uniq lines differ"
test $ni_u -eq $np_u || echoerror "uniq lines differ"

exit 0
