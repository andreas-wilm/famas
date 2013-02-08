famas - FastQ massaging
=======================

Yet another FASTQ trimming and filtering program. This one has proper
paired-end support and supports gzipped input as well as output (also
on stdin and stdout) and it's fast.

Started this little project, because my original Biopython
script (which implemented a filtering recipe similar to the one
described in Miller et al. (2011; PMID: 21595876)) was terribly slow.
Besides, I always wanted to use Heng Li's kseq.h.


Installation
============

You will need libz and libargtable2 installed on your system for
compilation. Download the latest package from the dist sub-folder and
do the GNU triple jump:

    ./configure
    make
    make install

If configure complains about missing libraries of which you are sure
they are installed, just in an unusual directory, modify your CFLAGS
and LDFLAGS environment variables accordingly. For example, if you
have libargtable2 installed via Macports which resides in /opt/local
you might want to use:

    ./configure CFLAGS='-I/opt/local/include'  LDFLAGS='-L/opt/local/lib'


Note: If you clone the git repository instead of downloading the
package, you will first have to run autoreconf and automake
--add-missing to set up the needed autotools files.

