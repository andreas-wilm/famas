famas - FastQ massaging
=======================

Yet another program for FastQ massaging:

Features:

- Quality- and length-based trimming
- Random sampling
- Splitting into multiple files
- Order checking for paired-end files
- Native gzip support
- It's fast!

Installation
============

Famas depends on [libz](http://www.zlib.net/) ,
[argtable3](http://www.argtable.org) and
[Heng Li's kseq](http://lh3lh3.users.sourceforge.net/kseq.shtml). The
latter two come with the famas source and libz is very likely already
installed in your system.

Unpack the source and do the GNU triple jump:

    ./configure
    make
    make install

If configure complains about missing libraries of which you are sure
they are installed, just in an unusual directory, modify your CFLAGS
and LDFLAGS environment variables accordingly. For example, if you
have libz installed via Macports which resides in /opt/local
you might want to use:

    ./configure CFLAGS='-I/opt/local/include'  LDFLAGS='-L/opt/local/lib'


Note: If you clone the git repository instead of downloading the
package, you will first have to run the following (just once) to set
up the autotools files (configure and Makefile.ins).

    autoreconf
    automake --add-missing

and possibly

    [g]libtoolize
    autoreconf




Usage
=====

famas -h will produce the following help:

    famas (0.0.8a) - yet another program for FAstq MASsaging
    
    Usage: famas [-fah] -i <file> [-j <file>] -o <file> [-p <file>] [--no-filter] [-Q <int>] [-q <int>] [-l <int>] [-e <33|64>] [-s <int>] [-x <int>] [--no-order-check] [--no-qual-check] [--quiet] [--debug]
    
    Files:
      -i, --in1=<file>          Input FastQ file (gzip supported; '-' for stdin)
      -j, --in2=<file>          Other input FastQ file if paired-end (gzip supported)
      -o, --out1=<file>         Output FastQ file (will be gzipped; '-' for stdout)
      -p, --out2=<file>         Other output FastQ file if paired-end input (will be gzipped)
    
    Trimming & Filtering:
      --no-filter               Switch all filtering off (none of the options in this sections apply then)
      -Q, --min5pqual=<int>     Trim from start/5'-end if base-call quality is below this value. Default: 0
      -q, --min3pqual=<int>     Trim from end/3'-end if base-call quality is below this value (Illumina guidelines recommend 3). Default: 3
      -l, --minlen=<int>        Discard reads if read length is below this length (discard both reads if either is below this limit). Default: 30
      -e, --phred=<33|64>       Qualities are ASCII-encoded Phred +33 (e.g. Sanger, SRA, Illumina 1.8+) or +64 (e.g. Illumina 1.3-1.7). Default: 33
    
    Sampling:
      -s, --sampling=<int>      Randomly sample roughly every <int>th read (after filtering, if used)
      -x, --split-every=<int>   Create new output after every <int'th> read. Requires "XXXXXX" in output names, which will be replaced with split number
    
    Checks:
      --no-order-check          Don't check paired-end read order (otherwise checked every 1000 reads)
      --no-qual-check           Don't check quality range (otherwise checked every 1000 reads)
    
    Misc:
      -f, --overwrite           Overwrite output files
      -a, --append              Append to output files
      -h, --help                Print this help and exit
      --quiet                   No output, except errors
      --debug                   Print debugging info
