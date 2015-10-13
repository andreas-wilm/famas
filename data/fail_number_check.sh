f1=SRR499813_1.Q2-and-N.fastq.gz
f2=SRR499813_2.Q2-and-N.fastq.gz
o1=${f1%.fastq.gz}.fail_number_check.fastq.gz
o2=${f2%.fastq.gz}.fail_number_check.fastq.gz

gzip -dc $f1 | head -n 40 | gzip > $o1
gzip -dc $f2 | head -n 20 | gzip > $o2
