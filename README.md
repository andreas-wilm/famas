fastq-filter
============

Yet another FASTQ filtering program. This one has proper paired-end
support and supports gzipped input (also stdin)  as well as gzipped
output (also stdout) and it's fast.

Started this little project, because the originally used Biopython
script (which implemented a filtering recipe similar to the one
described in Miller et al. (2011; PMID: 21595876)) was terribly slow.
Besides, I always wanted to use Heng Li's kseq.h.

You will need libz and libargtable2 installed on your system for
compilation.
