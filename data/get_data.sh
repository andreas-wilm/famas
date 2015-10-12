#!/bin/bash

test -e SRR499813_1.Q2-and-N.fastq.gz && exit 1
test -e SRR499813_2.Q2-and-N.fastq.gz && exit 1

wget -nd ftp://ftp.sra.ebi.ac.uk/vol1/fastq/SRR499/SRR499813/SRR499813_1.fastq.gz
wget -nd ftp://ftp.sra.ebi.ac.uk/vol1/fastq/SRR499/SRR499813/SRR499813_2.fastq.gz

./extract_some_with_q2_and_n.py
rm -f SRR499813_1.fastq.gz SRR499813_2.fastq.gz

