md5=$(which md5sum 2>/dev/null || which md5)
i=/Users/wilma/GIS/germs-git/vipr/tests/PDM003_1st1M_s1.fastq.gz;
j=/Users/wilma/GIS/germs-git/vipr/tests/PDM003_1st1M_s2.fastq.gz;
l=36
rm -f /tmp/s*gz;
/usr/bin/time -p ../tmp/emirge_pe_filter.py -t fastq --reads1 $i  --reads2 $j --out1 /tmp/s1.gz --out2 /tmp/s2.gz --min-read-len $l > time.old 2>&1 || exit 1
/usr/bin/time -p ../src/famas -l $l -i $i  -j $j -o /tmp/s1_n.gz -p /tmp/s2_n.gz > time.new 2>&1 || exit 1
origmd5=$(gzip -dc /tmp/s1_n.gz /tmp/s2_n.gz | md5)
newmd5=$(gzip -dc /tmp/s1.gz /tmp/s2.gz | md5)
test $origmd5 == $newmd5 || exit 1
