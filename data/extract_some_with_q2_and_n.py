#!/usr/bin/env

import os
import gzip
from itertools import izip_longest
from Bio import SeqIO


fqi1 = "SRR499813_1.fastq.gz"
fqi2 = "SRR499813_2.fastq.gz"
fqo1 = fqi1.replace(".fastq.gz", ".Q2-and-N.fastq.gz")
fqo2 = fqi2.replace(".fastq.gz", ".Q2-and-N.fastq.gz")


def extract_pairs(fh1, fh2):
    for sr1, sr2 in izip_longest(SeqIO.parse(fh1, "fastq"), SeqIO.parse(fh2, "fastq")):
        # make sure we have same number of reads in input
        assert sr1 is not None and sr2 is not None
            
        pq1 = sr1.letter_annotations["phred_quality"] 
        pq2 = sr2.letter_annotations["phred_quality"] 
        # want at least one base with BQ2 and at least one N in pair
        if 2 not in pq1 and 2 not in pq2:
            continue
        if 'N' not in sr1.seq and 'N' not in sr2:
            continue
        yield sr1, sr2

assert not os.path.exists(fqo1) and not os.path.exists(fqo2), (
    "Cowardly refusing to overwrite existing files")
with gzip.open(fqi1) as fhi1, gzip.open(fqi2) as fhi2:
    with gzip.open(fqo1, 'w') as fho1, gzip.open(fqo2, 'w') as fho2:
        for sr1, sr2 in extract_pairs(fhi1, fhi2):
            SeqIO.write([sr1], fho1, "fastq")
            SeqIO.write([sr2], fho2, "fastq")
