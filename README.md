fastq-filter
============

Yet another FASTQ filtering program.

This one has proper paired-end support and gzip-input as well as
-output support and it's fast.

Started this little project of frustration, because the originally
used Biopython script (which implemented a filtering recipe similar to
the onedescribed in Miller et al. (2011; PMID: 21595876)) was terribly
slow. Besides, I always wanted to use Heng Li's kseq.h.

You will need libz and libargtable2 installed on your system for
compilation.
