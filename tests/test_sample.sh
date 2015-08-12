source lib.sh || exit 1

i=./fastq-sanger/mux079-pdm003_s1.fastq.gz

num_orig=$(gzip -dc $i | grep -c '^@HWI')

#cat <<EOF
cmd="$famas --quiet --no-filter -s 2 -i $i -o -"
#echodebug "cmd = $cmd"
num_sample=$(eval $cmd | gzip -dc | grep -c '^@HWI')
#echodebug "num_orig=$num_orig num_sample=$num_sample cmd=$cmd"
if [ $num_sample -lt 20 ] || [ $num_sample -gt 80 ]; then
    echoerror "Number of sampled sequences out of range (might happen by chance though)"
    exit 1
fi

exit 0
