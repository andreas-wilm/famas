#!/bin/bash
#
# test split functionality
#


source lib.sh || exit 1


DEBUG=0
i=../data/SRR499813_1.Q2-and-N.fastq.gz
j=../data/SRR499813_2.Q2-and-N.fastq.gz
odir=$(mktemp -d -t $0..sh.XXX) || exit 1
#echodebug "odir=$odir"
o=$odir/o1.fastq.gz
p=$odir/o2.fastq.gz


cmd="$famas -i $i -o $o --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
step=1
#echodebug "step $step done"; let step=step+1

md5_i=$(gzip -dc $i | $md5)
md5_o=$(gzip -dc $o | $md5)
if [ $md5_i != $md5_o ]; then
    echoerror "Content changed even though we used no filter: compare $i and $o (command was $cmd)"
    exit 1
fi



stage="overwrite disabled"
cmd="$famas -i $i -o $o --quiet"
if eval $cmd 2>log.txt; then
    echoerror "The following command should have failed ($stage): $cmd"
    exit 1
fi
#echodebug "step $step done"; let step=step+1


# overwrite option should work.
stage="overwrite enabled"
cmd="$famas -i $i -o $o --quiet --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed ($stage): $cmd"
    exit 1
fi
#echodebug "step $step done"; let step=step+1


stage="create second file"
cmd="$famas -i $j -o $p --quiet"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed ($stage): $cmd"
    exit 1
fi
#echodebug "step $step done"; let step=step+1


# first and second should contain same number of reads
# since --no-fillter was used.
# should also contain the same number of reads as input
num_i=$(gzip -dc $i | wc -l)
num_j=$(gzip -dc $j | wc -l)
num_o=$(gzip -dc $o | wc -l)
num_p=$(gzip -dc $p | wc -l)
if [ $num_i != $num_j ]; then
    echoerror "INTERNAL ERROR: number of PE input reads differ for $i and $j"
    exit 1
fi
if [ $num_i != $num_o ]; then
    echoerror "Number of input and unfiltered output reads differ for $i and $o"
    exit 1
fi
if [ $num_o != $num_p ]; then
    echoerror "Number of unfiltered output reads differ for $o and $p"
    exit 1
fi
#echodebug "step $step done"; let step=step+1
rm -f $o $p


# set start -Q, end -q or len -l too low and you get nothing

# Q
cmd="$famas -i $i -j $j -o $o -p $p --quiet -5 255 --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
numl_o=$(gzip -dc $o | wc -l)
numl_p=$(gzip -dc $p | wc -l)
if [ $numl_o != 0 ] || [ $numl_p != 0 ] ; then
    echoerror "Q was set high so both of the following should contain no reads: $o and $p"
    echoerror "command was $cmd"
    exit 1
fi
#echodebug "step $step done"; let step=step+1


cmd="$famas -i $i -j $j -o $o -p $p --quiet -3 255 --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
numl_o=$(gzip -dc $o | wc -l)
numl_p=$(gzip -dc $p | wc -l)
if [ $numl_o != 0 ] || [ $numl_p != 0 ] ; then
    echoerror "q was set high so both of the following should be zero bytes: $o and $p"
    exit 1
fi
#echodebug "step $step done"; let step=step+1


cmd="$famas -i $i -j $j -o $o -p $p --quiet -l 1000 --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command failed: $cmd"
    exit 1
fi
numl_o=$(gzip -dc $o | wc -l)
numl_p=$(gzip -dc $p | wc -l)
if [ $numl_o != 0 ] || [ $numl_p != 0 ] ; then
    echoerror "l was set high so both of the following should be zero bytes: $o and $p"
    exit 1
fi
#echodebug "step $step done"; let step=step+1

    

# set all very low and you get same as input
#
cmd="$famas -i $i -j $j -o $o -p $p --quiet -5 0 -3 0 -l 0 --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command  failed: $cmd"
    exit 1
fi

md5_i=$(gzip -dc $i | $md5)
md5_j=$(gzip -dc $j | $md5)
md5_o=$(gzip -dc $o | $md5)
md5_p=$(gzip -dc $p | $md5)
if [ $md5_i != $md5_o ]; then
    echoerror "Content changed while splitting second file: compare $i and $o"
    exit 1
fi
if [ $md5_j != $md5_p ]; then
    echoerror "Content changed while splitting second file: compare $j and $p"
    exit 1
fi


# default should leave no reads <minlen bp and no Q2 bases '#' at ends
minlen=40
cmd="$famas -i $i -j $j -o $o -p $p -l $minlen --quiet -3 3 -5 3 --overwrite"
if ! eval $cmd 2>log.txt; then
    echoerror "The following command  failed: $cmd"
    exit 1
fi
num_o_q2_e=$(gzip -dc $o | awk 'NR%4==0' | grep -c '#$')
num_p_q2_e=$(gzip -dc $p | awk 'NR%4==0' | grep -c '#$')
num_o_q2_s=$(gzip -dc $o | awk 'NR%4==0' | grep -c '^#')
num_p_q2_s=$(gzip -dc $p | awk 'NR%4==0' | grep -c '^#')
if [ $num_o_q2_s -gt 0 ] || [ $num_p_q2_s -gt 0 ] || [ $num_o_q2_e -gt 0 ] || [ $num_p_q2_e -gt 0 ]; then
    echoerror "Found Q2 bases in $o or $p"
    exit 1
fi
min_len_o=$(gzip -dc $o | awk '{if (NR%4==2) {print length($0)}}' | sort | head -n 1)
min_len_p=$(gzip -dc $p | awk '{if (NR%4==2) {print length($0)}}' | sort | head -n 1)
if [ $min_len_o -lt $minlen ] || [ $min_len_p -lt $minlen ]; then
    echoerror "Found sequences <$minlen bp in $o or $p"
    exit 1
fi




if [ $DEBUG -eq 1 ]; then
    echodebug "Keeping $odir"
else
    test -d $odir && rm -rf $odir
fi
