source lib.sh || exit 1

i=./fastq-sanger/mux079-pdm003_s1.fastq.gz

md5orig=$(gzip -dc $i | $md5 | cut -f 1 -d ' ')

#cat <<EOF
cmd="$famas --quiet -f -l 30 --no-filter -i $i -o -"
#echodebug "cmd = $cmd"
md5out=$(eval $cmd | gzip -dc | $md5 | cut -f 1 -d ' ') || exit 1
if [ "$md5orig" != "$md5out" ]; then
    echoerror "Expected unchanged output with --no-filter"
    exit 1
fi

cmd="$famas --quiet -f -l 30 -i $i -o -"
#echodebug "cmd = $cmd"
md5out=$(eval $cmd | gzip -dc | $md5 | cut -f 1 -d ' ') || exit 1
if [ "$md5orig" == "$md5out" ]; then
    echoerror "Expected changed output with --no-filter"
    exit 1
fi

exit 0
