/* -*- c-file-style: "k&r" -*- 
 *
 * Copyright (C) 2013 Andreas Wilm <andreas.wilm@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include <zlib.h>

#include "argtable3/argtable3.h"
#include "kseq/kseq.h"


KSEQ_INIT(gzFile, gzread)

/* http://stackoverflow.com/questions/3437404/min-and-max-in-c */
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/* defaults:
 * most can be overwritten during compile time, e.g.
 *-DPAIRED_ORDER_SAMPLERATE=2
 */
#ifndef DEFAULT_MIN3PQUAL
#define DEFAULT_MIN3PQUAL 3
#endif
#ifndef DEFAULT_MIN5PQUAL
#define DEFAULT_MIN5PQUAL 0
#endif
#ifndef DEFAULT_MINREADLEN
#define DEFAULT_MINREADLEN 30
#endif
#ifndef DEFAULT_PHREDOFFSET
#define DEFAULT_PHREDOFFSET 33
#endif
#ifndef PAIRED_ORDER_SAMPLERATE
#define PAIRED_ORDER_SAMPLERATE 10000
#endif
#ifndef QUAL_CHECK_SAMPLERATE
#define QUAL_CHECK_SAMPLERATE 1000
#endif
#define EARLY_EXIT_MESSAGE "Don't trust already produced results. Exiting..."

#define TEMPLATE_MARK "XXXXXX"
#define TEMPLATE_FMT "%06d"

/* print macro argument as string */
#define XSTR(a) STR(a)
#define STR(a) #a


/* check if pointer is NULL and if true return 1 */
#define NULLCHECK(x) if(NULL==(x)) {fprintf(stderr, "FATAL(%s|%s:%d): memory allocation error\n", __FILE__, __FUNCTION__, __LINE__); return 1;}

typedef struct {
     char *infq1;
     char *infq2;
     char *outfq1;
     char *outfq2;

     int no_filter;
     int min5pqual;
     int min3pqual;
     int phredoffset;
     int minreadlen;

     int sampling;
     int split_every;

     int no_order_check;
     int no_qual_check;
     int overwrite_output;
     int append_to_output;
} args_t;


typedef struct {
     /* zero offset pos for trimming */
     int pos5p;
     int pos3p;
} trim_pos_t;


typedef struct {
     int min5pqual;
     int min3pqual;
     int minreadlen;
} trim_args_t;


/* Logging macros. You have to use at least one fmt+string. Newline
 * characters will not be appended automatically
 */
int verbose = 1;
int debug = 0;
#ifdef TRACE
int trace = 1;
#else
int trace = 0;
#endif


/* print only if debug is true*/

/* careful: for all of these you always need at least one fmt string */
#define LOG_DEBUG(fmt, args...)     {if (debug) {(void)vout(stderr, "DEBUG(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args);}}
/* print only if verbose is true*/
#define LOG_INFO(fmt, args...)      {if (verbose) {(void)vout(stderr, fmt, ## args);}}
/* always warn to stderr */
#define LOG_WARN(fmt, args...)      (void)vout(stderr, "WARNING(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args)
/* always print errors to stderr*/
#define LOG_ERROR(fmt, args...)     (void)vout(stderr, "ERROR(%s|%s:%d): " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)
#define LOG_FATAL(fmt, args...)     (void)vout(stderr, "FATAL(%s|%s:%d): " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)
/* always print fixme's */
#define LOG_FIXME(fmt, args...)     (void)vout(stderr, "FIXME(%s|%s:%d): " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)
#define LOG_TEST(fmt, args...)      (void)vout(stderr, "TESTING(%s|%s:%d): " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)


/* Taken from the Linux kernel source and slightly modified.
 */
int
vout(FILE *stream, const char *fmt, ...)
{                
     va_list args;
     int rc;
     
     va_start(args, fmt);
     rc = vfprintf(stream, fmt, args);
     va_end(args);
     return rc;
}


/* from 
 * http://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform 
 */
int file_exists(const char *fname) 
{
     if (access(fname, F_OK) != -1) {
          return 1;
     } else {
          return 0;
     }
}


void dump_args(const args_t *args) 
{
     LOG_DEBUG("%s\n", "args:");
     LOG_DEBUG("  infq1              = %s\n", args->infq1);
     LOG_DEBUG("  infq2              = %s\n", args->infq2);
     LOG_DEBUG("  outfq1             = %s\n", args->outfq1);
     LOG_DEBUG("  outfq2             = %s\n", args->outfq2);

     LOG_DEBUG("  no_filter          = %d\n", args->no_filter);
     LOG_DEBUG("  min5pqual          = %d\n", args->min5pqual);
     LOG_DEBUG("  min3pqual          = %d\n", args->min3pqual);
     LOG_DEBUG("  phredoffset        = %d\n", args->phredoffset);
     LOG_DEBUG("  minreadlen         = %d\n", args->minreadlen);

     LOG_DEBUG("  sampling           = %d\n", args->sampling);
     LOG_DEBUG("  split_every        = %d\n", args->split_every);

     LOG_DEBUG("  no_order_check     = %d\n", args->no_order_check);
     LOG_DEBUG("  no_qual_check      = %d\n", args->no_qual_check);
     LOG_DEBUG("  overwrite_output     = %d\n", args->overwrite_output);
     LOG_DEBUG("  append_to_output     = %d\n", args->append_to_output);
}


void free_args(args_t *args)
{
     free(args->infq1);
     args->infq1 = NULL;
     free(args->infq2);
     args->infq2 = NULL;
     free(args->outfq1);
     args->outfq1 = NULL;
     free(args->outfq2);
     args->outfq2 = NULL;
}


int template_mark_counts(const char *str)
{
     int mark_counts= 0;
     while (1) {
          str = strstr(str, TEMPLATE_MARK);
          if (!str) {
               break;
          }
          mark_counts++;
          str++;
     }
     return mark_counts;
}

/* Parses command line arguments and sets members in args accordingly.
 * Also sets defaults and performs logic checks. Returns -1 on error
 * Caller has to free args members using free_args(), even if -1 was
 * returned.
 */
int parse_args(args_t *args, int argc, char *argv[])
{
     int nerrors;

     /* argtable command line parsing:
      * http://argtable.sourceforge.net/doc/argtable2-intro.html
      *
      * basic structure is: arg_xxxN:
      * xxx can be int, lit, db1, str, rex, file or date
      * If N = 0, arguments may appear zero-or-once; N = 1 means
      * exactly once, N = n means up to maxcount times
      *
      * The only feature I miss in argtable is to be able to provide
      * the glossary with default values. A stringification macro
      * helps though.
      */  
     struct arg_rem  *rem_files  = arg_rem(NULL, "\nFiles:");
     struct arg_file *opt_infq1 = arg_file1(
          "i", "in1", "<file>",
          "Input FastQ file (gzip supported; '-' for stdin)");
     struct arg_file *opt_infq2 = arg_file0(
          "j", "in2", "<file>",
          "Other input FastQ file if paired-end (gzip supported)");
     struct arg_file *opt_outfq1 = arg_file1(
          "o", "out1", "<file>",
          "Output FastQ file (will be gzipped; '-' for stdout)");
     struct arg_file *opt_outfq2 = arg_file0(
          "p", "out2", "<file>",
          "Other output FastQ file if paired-end input (will be gzipped)");

     struct arg_rem  *rem_filtering  = arg_rem(NULL, "\nTrimming & Filtering:");
     struct arg_lit *opt_no_filter  = arg_lit0(
          NULL, "no-filter", 
          "Switch all filtering off (none of the options in this sections apply then)");
     struct arg_int *opt_min5pqual = arg_int0(
          "Q", "min5pqual", "<int>",
          "Trim from start/5'-end if base-call quality is below this value."
          " Default: " XSTR(DEFAULT_MIN5PQUAL));
     struct arg_int *opt_min3pqual = arg_int0(
          "q", "min3pqual", "<int>",
          "Trim from end/3'-end if base-call quality is below this value (Illumina guidelines recommend 3)."
          " Default: " XSTR(DEFAULT_MIN3PQUAL));
     struct arg_int *opt_phredoffset = arg_int0(
          "e", "phred", "<33|64>",
          "Qualities are ASCII-encoded Phred +33 (e.g. Sanger, SRA,"
          " Illumina 1.8+) or +64 (e.g. Illumina 1.3-1.7)."
          " Default: " XSTR(DEFAULT_PHREDOFFSET));
     struct arg_int *opt_minreadlen = arg_int0(
          "l", "minlen", "<int>",
          "Discard reads if read length is below this length (discard both reads if"
          " either is below this limit). Default: " XSTR(DEFAULT_MINREADLEN));

     struct arg_rem *rem_sampling = arg_rem(NULL, "\nSampling:");
     struct arg_int *opt_sampling = arg_int0(
          "s", "sampling", "<int>",
          "Randomly sample roughly every <int>th read (after filtering, if used)");
     struct arg_int *opt_split_every = arg_int0(
          "x", "split-every", "<int>",
          "Split every x reads. Requires " TEMPLATE_MARK " in output names, which will be replaced with split number");

     struct arg_rem  *rem_checks  = arg_rem(NULL, "\nChecks:");
     struct arg_lit *opt_no_order_check = arg_lit0(
          NULL, "no-order-check", 
          "Don't check paired-end read order (otherwise checked every " XSTR(PAIRED_ORDER_SAMPLERATE) " reads)");
     struct arg_lit *opt_no_qual_check  = arg_lit0(
          NULL, "no-qual-check", 
          "Don't check quality range (otherwise checked every " XSTR(QUAL_CHECK_SAMPLERATE) " reads)");

     struct arg_rem  *rem_misc  = arg_rem(NULL, "\nMisc:");
     struct arg_lit *opt_overwrite_output  = arg_lit0(
          "f", "overwrite", 
          "Overwrite output files");
     struct arg_lit *opt_append_to_output  = arg_lit0(
          "a", "append", 
          "Append to output files");
     struct arg_lit *opt_help = arg_lit0(
          "h", "help",
          "Print this help and exit");
     struct arg_lit *opt_quiet  = arg_lit0(
          NULL, "quiet", 
          "No output, except errors");
     struct arg_lit *opt_debug  = arg_lit0(
          NULL, "debug", 
          "Print debugging info");
     struct arg_end *opt_end = arg_end(20); /* maximum number of errors
                                             * to store */
     
     /* set defaults 
      */
     opt_minreadlen->ival[0] = DEFAULT_MINREADLEN;
     opt_min5pqual->ival[0] = DEFAULT_MIN5PQUAL;
     opt_min3pqual->ival[0] = DEFAULT_MIN3PQUAL;
     opt_phredoffset->ival[0] = DEFAULT_PHREDOFFSET;
     opt_split_every->ival[0] = 0;
     opt_sampling->ival[0] = 0;

     void *argtable[] = {rem_files, opt_infq1, opt_infq2, opt_outfq1, opt_outfq2,
                         rem_filtering, opt_no_filter, opt_min5pqual, opt_min3pqual, opt_minreadlen, opt_phredoffset, 
                         rem_sampling, opt_sampling, opt_split_every,
                         rem_checks, opt_no_order_check, opt_no_qual_check, 
                         rem_misc, opt_overwrite_output, opt_append_to_output, opt_help, opt_quiet, opt_debug,
                         opt_end};    
     
     if (arg_nullcheck(argtable) != 0) {
          LOG_ERROR("%s\n", "insufficient memory for argtable allocation.");
          return 1;
     }

     nerrors = arg_parse(argc, argv, argtable);

     /* Special case: '--help' takes precedence over error reporting
      * and we are allowed to exit from here */
     if (opt_help->count > 0) {
          fprintf(stderr, "%s (%s) - yet another program for FAstq MASsaging\n\n",
                  PACKAGE_NAME, PACKAGE_VERSION);
          fprintf(stderr, "Usage: %s", PACKAGE_NAME);
          arg_print_syntax(stderr, argtable, "\n");
          arg_print_glossary(stderr, argtable, "  %-25s %s\n");
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          exit(EXIT_SUCCESS);
     }
     
     if (nerrors > 0) {
          arg_print_errors(stdout, opt_end, PACKAGE_NAME);
          fprintf(stderr, "For more help try: %s -h or --help\n", PACKAGE_NAME);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;
     }
     

     args->no_filter = opt_no_filter->count;
     args->no_order_check = opt_no_order_check->count;
     args->no_qual_check = opt_no_qual_check->count;

     args->overwrite_output = opt_overwrite_output->count;
     args->append_to_output = opt_append_to_output->count;
     if (args->overwrite_output && args->append_to_output) {
          LOG_ERROR("%s\n", "Can't append and overwrite at the same time");
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;
     }
     if (opt_quiet->count) {
          verbose = 0;
     }
     debug = opt_debug->count;
     if (debug) {
          verbose = 1;
     }


     args->infq1 = strdup(opt_infq1->filename[0]);
     if (0 != strncmp(args->infq1, "-", 1) && ! file_exists(args->infq1)) {
          LOG_ERROR("File %s does not exist\n", args->infq1);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;
     }

     args->outfq1 = strdup(opt_outfq1->filename[0]);
     if (opt_infq2->count) {
          args->infq2 = strdup(opt_infq2->filename[0]);
          if (0 == strcmp(args->infq2, args->infq1)) {
               LOG_ERROR("%s\n", "The two input FastQ files are the same file");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return 1;              
          }

          if (! file_exists(args->infq2)) {
               LOG_ERROR("File %s does not exist\n", args->infq2);
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return 1;
          }
          
          if (! opt_outfq2->count) {
               LOG_ERROR("%s\n", "Need two output files for paired-end input");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return 1;                        
          }

          args->outfq2 = strdup(opt_outfq2->filename[0]);

     } else {
          if (opt_outfq2->count) {
               LOG_ERROR("%s\n", "Got second output file, not a corresponding second input file");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return 1;              
          }
     }

     args->min5pqual = opt_min5pqual->ival[0];
     if (args->min5pqual<0) {
          LOG_ERROR("Invalid 5' quality '%d'\n", args->min5pqual);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;            
     }

     args->min3pqual = opt_min3pqual->ival[0];
     if (args->min3pqual<0) {
          LOG_ERROR("Invalid 3' quality '%d'\n", args->min3pqual);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;            
     }

     args->phredoffset = opt_phredoffset->ival[0];
     if (33 != args->phredoffset && 64 != args->phredoffset) {
          LOG_ERROR("Invalid Phred-quality ASCII offset '%d'\n", args->phredoffset);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;            
     }

     
     args->minreadlen = opt_minreadlen->ival[0];
#if 0 /* negative values okay. just means no read length filter */
     if (args->minreadlen<0) {
          LOG_ERROR("Invalid length '%d'\n", args->minreadlen);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;            
     }
#endif

     args->sampling = opt_sampling->ival[0];
#if 0 /* negative values okay. just means no sampling */
     if (args->sampling<0) {
          LOG_ERROR("Invalid sampling setting '%d'\n", args->sampling);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return 1;            
     }
#endif

     args->split_every = opt_split_every->ival[0];
     if (args->split_every>0) {
          if (1 != template_mark_counts(args->outfq1)) {
               LOG_ERROR("Need %s exactly once as number template in output filename for requested splitting\n", TEMPLATE_MARK);
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));               
               return 1;
          }
          if (args->outfq2) {
               if (1 != template_mark_counts(args->outfq2)) {
                    LOG_ERROR("Need %s as number template in output filename for requested splitting\n", TEMPLATE_MARK);
                    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));               
                    return 1;
               }
          }
     }

     /* Whew! */

     arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
     return 0;
}


void dump_trim_pos(const trim_pos_t *tp)
{
     LOG_DEBUG("trim_pos pos5p=%d\n", tp->pos5p)
     LOG_DEBUG("trim_pos pos3p=%d\n", tp->pos3p)
}


/* returns 1 if read is to be discarded, in which case trim_pos
 * values might be set to arbitrary values. otherwise trim_pos will
 * hold valid (zero-offset) trimming positions.
 */
int calc_trim_pos(trim_pos_t *trim_pos, 
                  const kseq_t *seq, const int phredoffset,
                  const trim_args_t *trim_args)
{
     int i;
     int minreadlen;
     int trace = 0; /* local trace, overwriting global */

     trim_pos->pos5p = -1; /* internal flag */
     trim_pos->pos3p = -1; /* internal flag */

     /* check for neg minreadlen, otherwise logic below won't work */
     if (trim_args->minreadlen >= 1) {
          minreadlen = trim_args->minreadlen;
     } else {
          minreadlen = 1;
     }

     if (minreadlen > seq->qual.l) {
          LOG_DEBUG("%s\n", "Input read already smaller than minreadlen");
          return 1;
     }

     /* 3p end. test first, since more likely to be used by user
      */
     if (trim_args->min3pqual>0) {
          for (i=seq->qual.l-1; i >= minreadlen-1; i--) {
               int q = seq->qual.s[i]-phredoffset;
               assert(i>=0);
               if (trace) {LOG_DEBUG("Checking at 3p pos %d/%d if q %d >= %d\n",
                                     i, seq->qual.l, q, trim_args->min3pqual);}
               if (q >= trim_args->min3pqual) {
                    trim_pos->pos3p = i;
                    break;
               }
          }
          if (trim_pos->pos3p == -1) {
               if (trace) {LOG_DEBUG("%s\n", "trim_pos->pos3p == -1");}
               return 1;
          }
     } else {
          trim_pos->pos3p = seq->qual.l-1; /* zero offset */
     }

     /* 5p end 
      */
     if (trim_args->min5pqual>0) {
          for (i=0; i<seq->qual.l - minreadlen + 1 && i<=trim_pos->pos3p; i++) {
               int q = seq->qual.s[i] - phredoffset;
               assert(i<seq->qual.l);
               
               if (trace) {LOG_DEBUG("Checking at 5p pos %d/%d if q %d >= %d\n", 
                                     i, seq->qual.l, q, trim_args->min5pqual);}
               if (q >= trim_args->min5pqual) {
                    trim_pos->pos5p = i;
                    break;
               }
          }
          if (trim_pos->pos5p == -1) {
               if (trace) {LOG_DEBUG("%s\n", "trim_pos->pos5p == -1");}
               return 1;
          }
     } else {
          trim_pos->pos5p = 0;
     }
      
     /* test should be unnecessary if loops above are done
      * correctly */
     if (trim_pos->pos3p - trim_pos->pos5p + 1 < minreadlen) {
          if (trace) {LOG_DEBUG("%s\n", "trim_pos->pos3p - trim_pos->pos5p + 1 < minreadlen");}
          return 1;
     }
          
     /*LOG_DEBUG("Returning trim_pos->pos3p = %d  trim_pos->pos5p = %d\n", trim_pos->pos3p,  trim_pos->pos5p);*/
     return 0;
}


/* prints fastq entry including trailing newline. returns negative
 * number on error. caller has to free buf, which will be allocated
 * here. if trim_pos is not NULL or (both values are not -1) seq will
 * be trimmed accordingly.
 */
int sprintf_fastq(char **buf, const kseq_t *seq, const trim_pos_t *trim_pos) {
     int ret = 0;
     /* remember values temporarily overwritten during trimming */
     char zeroed_base = -1;
     char zeroed_qual = -1;
     int bufsize = 1/*@*/ + seq->name.l + 1/*space*/ + seq->comment.l
          + 1 /* newline */
          + seq->seq.l 
          + 3 /* newline, '+' and newline */ 
          + seq->qual.l 
          + 1 /* newline */
          + 1;  /* trailing 0 */
     int start = 0;

     /* fastq is supposed to have a quality string */
     if (! seq->qual.l){
          LOG_ERROR("%s\n", "FastQ is missing a quality string");
          return -1;
     }

     if (NULL != trim_pos && (trim_pos->pos5p >= 0 && trim_pos->pos3p >= 0)) {
          if (trim_pos->pos5p < 0 || trim_pos->pos3p < 0) {
               LOG_ERROR("%s\n", "Internal error: Invalid trim pos (5p or 3p < 0)");
               return -1;
          }
          if (trim_pos->pos3p - trim_pos->pos5p + 1 < 0) {
               LOG_ERROR("%s\n", "Internal error: Invalid trim pos (negative distance between 5p and 3p)");
               return -1;               
          }
          if (trim_pos->pos3p >= seq->qual.l) {
               LOG_ERROR("Internal error: Invalid 3p trim pos (%d > string length which is %d)\n",
                         trim_pos->pos3p, seq->qual.l);
               return -1;               
          }

          start = trim_pos->pos5p;
          /* temporarily overwrite the end */
          zeroed_base = seq->seq.s[trim_pos->pos3p+1];
          zeroed_qual = seq->qual.s[trim_pos->pos3p+1];
          seq->seq.s[trim_pos->pos3p+1] = '\0';
          seq->qual.s[trim_pos->pos3p+1] = '\0';
     }
     
     (*buf) = calloc(bufsize, sizeof(char));
     NULLCHECK(buf);
     
     /* or simply use asprintf, which is GNU extension available also on BSD */
     if (seq->comment.l) {
          ret = sprintf((*buf), "@%s %s\n%s\n+\n%s\n", 
                        seq->name.s, seq->comment.s, & seq->seq.s[start], & seq->qual.s[start]);
     } else {
          ret = sprintf((*buf), "@%s\n%s\n+\n%s\n", 
                        seq->name.s, & seq->seq.s[start], & seq->qual.s[start]);
     }

     if (NULL != trim_pos && (trim_pos->pos5p >= 0 && trim_pos->pos3p >= 0)) {
          /* revert temporary trimming changes */
          seq->seq.s[trim_pos->pos3p+1] = zeroed_base;
          seq->qual.s[trim_pos->pos3p+1] = zeroed_qual;
     }
     return ret;
}


/* same as sprintf_fastq but gzipped.
 */
int gzprintf_fastq(gzFile fp, const kseq_t *seq, const trim_pos_t *trim_pos) {
     char *buf;
     int ret;

     ret = sprintf_fastq(&buf, seq, trim_pos);
     if (ret<0) {
          LOG_ERROR("%s\n", "Couldn't format seq...");
          return ret;
     }
     ret = gzprintf(fp, "%s", buf);
     free(buf);

     return ret;
 }


/* returns 0 if not paired, 1 if reads are paired
 */
int reads_are_paired(const kseq_t *seq1, const kseq_t *seq2) {
     /* Either read names end in '/[12]$' (older illumina/casava) or
      * they contain ' [12]:[NY]:' at the right end. In the latter
      * case seq puts the last bit into seq.comment. Simulated reads
      * might end in '\.[12]'. Either way, compare only the first bit
      *
      * Examples: 
      *
      * @HWUSI-EAS100R:6:73:941:1973#0/1
      * @HWUSI-EAS100R:6:73:941:1973#0/2
      *
      * @B2GA005:3:1:3597:949#ACAGTG/1
      * @B2GA005:3:1:3597:949#ACAGTG/2
      *
      * @HWI-ST740:1:C0JMGACXX:1:1101:1452:2203 1:N:0:ATCACG
      * @HWI-ST740:1:C0JMGACXX:1:1101:1452:2203 2:N:0:ATCACG
      *
      */
     if (seq1->name.l != seq2->name.l) {
          return 0;
     }
     /* if we have a comment we assume seq put the last bit into
      * comment. otherwise we assume old illumina/casava with name
      * which endswith '/[12]$' */
     if (seq1->comment.l && seq2->comment.l) {
          return ! strcmp(seq1->name.s, seq2->name.s);

     } else {
          if (seq1->name.l < 3) {
               return 0;
          }
          /* ignore the '/[12]$' bit for comparison */
          return ! strncmp(seq1->name.s, seq2->name.s, seq1->name.l-2);
     }

#if 0     
     /* if name contained everything */
     int cmp_len = 0;
     int i;
     for (i=seq1->name.l-2; i>=0; i--) {
          char seq1_c = seq1->name.s[i];
          char seq2_c = seq2->name.s[i];
          if (seq1_c==' ' || seq1_c=='/' || seq1_c=='.') {
               if (seq1_c != seq2_c) {
                    return -1;
               }
               cmp_len = i+1;
               break;
          }
     }
     if (0 == cmp_len) {
          return -1;
     }
     if (0 == strncmp(seq1->name.s, seq2->name.s, cmp_len)) {
          return 1;
     } else {
          return 0;
     }
#endif
}


int test()
{
     int phredoffset = 33;
     char *buf;
     const int ks_bufsize = 1024;
     trim_pos_t trim_pos;
     kseq_t *ks;
     int orig_read_len;
     trim_args_t trim_args;

     ks = (kseq_t*)calloc(1, sizeof(kseq_t));
     NULLCHECK(ks);

     LOG_TEST("%s\n", "Starting interal tests");

     /* setup dummy kseq with enough space for some experiments.
      * WARNING: not sure if I used kseq_t correctly here
      */
     ks->name.l = ks->comment.l = ks->seq.l = ks->qual.l = 0;
     ks->name.m =  ks->comment.m = ks->seq.m = ks->qual.m = ks_bufsize;
     ks->name.s = (char*)realloc(ks->name.s, ks->name.m);
     ks->comment.s = (char*)realloc(ks->comment.s, ks->comment.m);
     ks->seq.s = (char*)realloc(ks->seq.s, ks->seq.m);
     ks->qual.s = (char*)realloc(ks->qual.s, ks->qual.m);
     /* now l has to be set after filling s */

     strcpy(ks->name.s, "@HWI-ST740:1:C0JMGACXX:1:1101:2161:2062 2:N:0:ATCACG");
     ks->name.l = strlen(ks->name.s);
     strcpy(ks->seq.s,  "AAACCCGGGTTTACGTAAACCCGGGTTTACGTAAACCCGGGTTTACGTAAAC");
     ks->seq.l = strlen(ks->seq.s);
     strcpy(ks->qual.s, "?@AAABBBCCCDDDEEEFFFGGGH????HGGGFFFEEEDDDCCCBBBAAA@?");
     ks->qual.l = strlen(ks->qual.s);
     orig_read_len = strlen(ks->seq.s);


     trim_args.min5pqual = 39;
     trim_args.min3pqual = 39;
     trim_args.minreadlen = 6;

     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }

     sprintf_fastq(&buf, ks, NULL);
     free(buf);
     if (ks->seq.l != orig_read_len || strlen(ks->seq.s) != orig_read_len) {
          LOG_ERROR("%s\n", "Read trimming changed read seq");
          kseq_destroy(ks);
          return 1;
     }

     if (ks->qual.l != ks->seq.l || strlen(ks->qual.s) != orig_read_len) {
          LOG_ERROR("%s\n", "Read trimming changed read qual");
          kseq_destroy(ks);
          return 1;
     }

     trim_args.minreadlen = 7;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     
     trim_args.min5pqual = 40;
     trim_args.min3pqual = 0;
     trim_args.minreadlen = 1;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }

     trim_args.min5pqual = 0;
     trim_args.min3pqual = 40;
     trim_args.minreadlen = 1;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }

     trim_args.min5pqual = 0;
     trim_args.min3pqual = 0;
     trim_args.minreadlen = 100;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }

     trim_args.min5pqual = 0;
     trim_args.min3pqual = 0;
     trim_args.minreadlen = 1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }

     strcpy(ks->qual.s, "???????????????????????????????????????????????????A");
     trim_args.min5pqual = 31;
     trim_args.min3pqual = 2;
     trim_args.minreadlen = 2;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.minreadlen = 1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }

     strcpy(ks->qual.s, "A????????????????????????????????????????????????????");
     trim_args.min5pqual = 2;
     trim_args.min3pqual = 31;
     trim_args.minreadlen = 2;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.minreadlen = 1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }


     strcpy(ks->seq.s,  "ACGT");
     ks->seq.l = strlen(ks->seq.s);
     strcpy(ks->qual.s, "5678"); /* Q = 20 21 22 23 */
     ks->qual.l = strlen(ks->qual.s);
     /*orig_read_len = strlen(ks->seq.s);*/
     trim_args.min5pqual = 24;
     trim_args.min3pqual = 0;
     trim_args.minreadlen = -1;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.min5pqual = 0;
     trim_args.min3pqual = 24;
     trim_args.minreadlen = -1;
     if (! calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("Read should have been discarded but is not. Got trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.min5pqual = 23;
     trim_args.min3pqual = 23;
     trim_args.minreadlen = -1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }
     if (trim_pos.pos5p!=3 || trim_pos.pos3p!=3) {
          LOG_ERROR("Got wrong trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.min5pqual = 23;
     trim_args.min3pqual = 0;
     trim_args.minreadlen = 1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }
     if (trim_pos.pos5p!=3 || trim_pos.pos3p!=3) {
          LOG_ERROR("Got wrong trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }
     trim_args.min5pqual = 0;
     trim_args.min3pqual = 23;
     trim_args.minreadlen = 1;
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_ERROR("%s\n", "Read was discarded even though it's okay");
          kseq_destroy(ks);
          return 1;
     }
     if (trim_pos.pos5p!=0 || trim_pos.pos3p!=3) {
          LOG_ERROR("Got wrong trim_pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          kseq_destroy(ks);
          return 1;
     }


#if 0
      *
      * @HWUSI-EAS100R:6:73:941:1973#0/1
      *
      * @B2GA005:3:1:3597:949#ACAGTG/1
      * @B2GA005:3:1:3597:949#ACAGTG/2
      *
      * @HWI-ST740:1:C0JMGACXX:1:1101:1452:2203 1:N:0:ATCACG
      * @HWI-ST740:1:C0JMGACXX:1:1101:1452:2203 2:N:0:ATCACG
      *
#endif
#if 0
     sprintf_fastq(&buf, ks, &trim_pos);
     LOG_FIXME("%s\n", buf);
     free(buf);
#endif

#if 0 
     sprintf_fastq(&buf, ks, NULL);
     fprintf(stderr, "--- seq before trimming:\n%s", buf);
     free(buf);

     for (i=0; i<ks->qual.l; i++) {
          int q = ks->qual.s[i] - phredoffset;
          fprintf(stderr, "%02d:%c:%d ", i, ks->seq.s[i], q);
          if ((i+1)%9==0) {
               fprintf(stderr, "\n");
          }
     }
     fprintf(stderr, "\n");

     LOG_WARN("Getting trim pos for read (len=%d) with min5pqual=%d min3pqual=%d minreadlen=%d\n",
              ks->seq.l, trim_args.min5pqual, trim_args.min3pqual, trim_args.minreadlen);
     if (calc_trim_pos(&trim_pos, ks, phredoffset, &trim_args)) {
          LOG_WARN("%s\n", "Read is to be discarded");
     } else {
          fprintf(stderr, "Got trim pos %d %d\n", trim_pos.pos5p, trim_pos.pos3p);
          sprintf_fastq(&buf, ks, &trim_pos);
          fprintf(stderr, "--- trimmed seq:\n%s", buf);          
          free(buf);
     }

     sprintf_fastq(&buf, ks, NULL);
     fprintf(stderr, "--- seq restored after trimming:\n%s", buf);
     free(buf);
#endif

     kseq_destroy(ks);
     LOG_TEST("%s\n", "Successfully completed");
     return EXIT_SUCCESS;
}


/* returns 1 if valid and 0 if invalid. using lenient definition.
 */
int qual_range_is_valid(const kseq_t *seq, const int phredoffset)
{
     /* FIXME faster to just check extreme values */
     int i;
     for (i=0; i<seq->qual.l; i++) {
          int q = seq->qual.s[i]-phredoffset;
          if (q<0 || q>93) {
               return 0;
          }
     }
     /* no qualities at all? */
     if (0 == i) {
          return 0;
     }
     return 1;
}


/* caller has to free out */
int replace_template_mark_with_no(char *in, char **out, int split_no) {
     char *template_mark = strstr(in, TEMPLATE_MARK);
     char repl_str[1024];
     int i;
     int pos;

     if (! template_mark) {
          LOG_ERROR("%s\n", "template mark not found\n");
          return 1;
     }
     if (split_no >= pow(10, strlen(TEMPLATE_MARK))) {
          LOG_ERROR("%s\n", "split number too large\n");
          return 1;
     }
     (*out) = strdup(in);
     sprintf(repl_str, TEMPLATE_FMT, split_no);
     
     pos = template_mark-in;
     for (i=0; i<strlen(TEMPLATE_MARK); i++) {
          (*out)[pos+i] = repl_str[i];
     }

     return 0;
}


int open_output_one(gzFile **fp_outfq, char *outfq, 
                    int append, int overwrite, int split_no) {
     char *fname = NULL;
     char outmode[2];/* output mode for both fq files */
     strcpy(outmode, "w");
     if (append) {
          strcpy(outmode, "a");
     }    

     if (0 == strcmp(outfq, "-")) {
          if (split_no > 0) {
               LOG_FATAL("%s\n", "Split with stdout as output not possible");
               return 1;
          }
          (*fp_outfq) = gzdopen(fileno(stdout), "w");
     } else {
          if (append) {
               strcpy(outmode, "a");
          }
          if (split_no > 0) {
               if (replace_template_mark_with_no(outfq, &fname, split_no)) {
                    return 1;
               }
          } else {
               fname = outfq;
          }
          
          if (file_exists(fname) && (! overwrite && ! append)) {
               LOG_ERROR("Cowardly refusing to overwrite existing file %s\n", fname);
               return 1;
               if (fname != outfq) {
                    free(fname);
               }
          }
          
          (*fp_outfq) = gzopen(fname, outmode);                    
          LOG_DEBUG("opening fname=%s for split_no=%d\n", fname, split_no);
     }

     if (NULL == (*fp_outfq)) {
          LOG_ERROR("Couldn't open %s\n", fname);
          return 1;
     }
     if (fname != outfq) {
          free(fname);
     }

     return 0;
}


/* fq1 one might be stdout. fq2 might be NULL. split_no used if >0 */
int open_output(gzFile **fp_outfq1, gzFile **fp_outfq2, 
                char *outfq1, char *outfq2, 
                int append, int overwrite, int split_no)
{
     int rc;

     if (trace) {LOG_DEBUG("open_output(): fp_outfq1=%p fp_outfq2=%p outfq1=%s outfq2=%s append=%d overwrite=%d split_no=%d\n", 
                           fp_outfq1, fp_outfq2, outfq1, outfq2, append, overwrite, split_no);}

    rc = open_output_one(fp_outfq1, outfq1, append, overwrite, split_no);
    if (rc) {
         return rc;
    }
    
    if (outfq2) {
         rc = open_output_one(fp_outfq2, outfq2, append, overwrite, split_no);
         if (rc) {
              return rc;
         }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    args_t args = { 0 };
    gzFile *fp_infq1 = NULL, *fp_infq2 = NULL;
    gzFile *fp_outfq1 = NULL, *fp_outfq2 = NULL;
    kseq_t *seq1 = NULL, *seq2 = NULL;
    int len1 = -1, len2 = -1;
    int pe_mode = 0; /* bool paired end mode */
    unsigned long int n_reads_in = 0, n_reads_out = 0; /* number of reads or pairs */
    trim_args_t trim_args;
    int rc;
    int read_order_warning_issued = 0;
    trim_pos_t *trim_pos_1 = NULL;
    trim_pos_t *trim_pos_2 = NULL;
    
#ifdef TEST
    return test();
#endif
    srand(time(NULL));

   
    if (parse_args(&args, argc, argv)) {
         free_args(& args);
         return EXIT_FAILURE;
    }
    if (debug) {
         dump_args(& args);
    }
    trim_args.min5pqual = args.min5pqual;
    trim_args.min3pqual = args.min3pqual;
    trim_args.minreadlen = args.minreadlen;
    if (args.infq2) {
         pe_mode = 1;
    }

    /* open input fqs
     */
    fp_infq1 = (0 == strcmp(args.infq1, "-")) ? 
         gzdopen(fileno(stdin), "r") : gzopen(args.infq1, "r");
    if (NULL == fp_infq1) {
         LOG_ERROR("%s\n", "Couldn't open %s. Exiting...", args.infq1);
         free_args(& args);
         return EXIT_FAILURE;
    }
	if (pe_mode) {
         fp_infq2 = gzopen(args.infq2, "r");
         if (NULL == fp_infq2) {
              LOG_ERROR("%s\n", "Couldn't open %s. Exiting...", args.infq2);
              free_args(& args);
              return EXIT_FAILURE;
         }
    }


    if (open_output(&fp_outfq1, &fp_outfq2,
                    args.outfq1, args.outfq2,
                    args.append_to_output, args.overwrite_output, args.split_every>0? 1:0)) {
         LOG_ERROR("%s\n", "Couldn't open output files. Exiting...");
         free_args(& args);
         kseq_destroy(seq1);
         return EXIT_FAILURE;
    }         

	seq1 = kseq_init(fp_infq1);
	if (pe_mode) {
         seq2 = kseq_init(fp_infq2);
    }
    n_reads_in = n_reads_out = 0;
    if (! args.no_filter) {
         trim_pos_1 = malloc(sizeof(trim_pos_t));
         trim_pos_2 = malloc(sizeof(trim_pos_t));
    }
	while ((len1 = kseq_read(seq1)) >= 0) {
         if (trace) {LOG_DEBUG("Inspecting seq1: %s\n", seq1->name.s);}

         if (pe_mode) {
              /* read read2 directly to keep both in sync. a continue
               * before reading read2 makes order invalid */
              if ((len2 = kseq_read(seq2)) < 0) {
                   LOG_ERROR("Reached premature end in second file (%s)."
                             " Still received reads from first file (%s from %s). %s\n",
                             args.infq2, seq1->name.s, args.infq1, EARLY_EXIT_MESSAGE);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
              if (trace) {LOG_DEBUG("Inspecting seq2: %s\n", seq2->name.s);}
         }
         
         n_reads_in+=1;
         if (0 == n_reads_in%100000) {
              LOG_DEBUG("Still alive and happily massaging read %d\n", n_reads_in);
         }

         if (! args.no_filter) {
              trim_pos_1->pos5p = trim_pos_1->pos3p = 1<<20; /* make invalid */
              trim_pos_2->pos5p = trim_pos_2->pos3p = 1<<20; /* make invalid */
         }

         /* quality check: done before trimming to see more reads
          */
         if (! args.no_qual_check && 1 == (n_reads_in%QUAL_CHECK_SAMPLERATE)) {
              if (! qual_range_is_valid(seq1, args.phredoffset)) {
                   LOG_ERROR("Read %s has qualities outside valid range (%s). %s\n",
                             seq1->name.s, seq1->qual.s, EARLY_EXIT_MESSAGE);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }

         if (! args.no_filter && calc_trim_pos(
                  trim_pos_1, seq1, args.phredoffset, &trim_args)) {
              if (trace) {LOG_DEBUG("%s\n", "seq1 to be discarded");}
              continue;
         }


         if (pe_mode) {
              if (! args.no_qual_check && 1 == (n_reads_in%QUAL_CHECK_SAMPLERATE)) {
                   if (! qual_range_is_valid(seq1, args.phredoffset)) {
                        LOG_ERROR("Read %s has qualities outside valid range (%s). %s\n",
                                  seq1->name.s, seq1->qual.s, EARLY_EXIT_MESSAGE);
                        rc = EXIT_FAILURE;
                        goto free_and_exit;
                   }
              }

              if (! args.no_filter && calc_trim_pos(
                       trim_pos_2, seq2, args.phredoffset, &trim_args)) {
                   if (trace) {LOG_DEBUG("%s\n", "seq2 to be discarded");}
                   continue;
              }
              
              /* read order check (PE only)
               */
              if (! args.no_order_check && ! read_order_warning_issued
                  && (1 == (n_reads_in%PAIRED_ORDER_SAMPLERATE))) {
                   rc = reads_are_paired(seq1, seq2);
                   if (1 != rc) {
                        if (0 == rc) {
                             LOG_ERROR("Read order check failed."
                                       " Checked reads names were %s and %s. %s\n",
                                       seq1->name.s, seq2->name.s, EARLY_EXIT_MESSAGE);
                             rc = EXIT_FAILURE;
                             goto free_and_exit;
           
                        } else if (-1 == rc) {
                             LOG_WARN("Couldn't derive read order from reads"
                                      " %s and %s. Continuing anyway...\n",
                                      seq1->name.s, seq2->name.s);
                             read_order_warning_issued = 1;
                        }
                   }
                   LOG_DEBUG("read order okay for %s and %s\n", 
                             seq1->name.s, seq2->name.s);
              }
         }

         if (args.sampling>1) {
              /* random int ranging from 1 to args.sampling */
              int r = rand()%args.sampling+1; 
              if (r != args.sampling) {
                   continue;
              }
         }


         /* 
          * output 
          *
          */

         if (args.split_every>0) {
              /* for split every we reopen files if necessary */
              if ((n_reads_out+1)%args.split_every == 0) {
                   gzclose(fp_outfq1);
                   if (pe_mode) {
                        gzclose(fp_outfq2);
                   }
                   if (open_output(&fp_outfq1, &fp_outfq2,
                                   args.outfq1, args.outfq2,
                                   args.append_to_output, args.overwrite_output,
                                   (n_reads_out+1)/args.split_every+1)) {
                        LOG_ERROR("%s\n", "Couldn't open output files. Exiting...");
                        return EXIT_FAILURE;
                   }         
              }
              
         }
              

         if (0 >= gzprintf_fastq(fp_outfq1, seq1, trim_pos_1)) {
              LOG_ERROR("Couldn't write to %s (after successfully writing"
                        "  %d reads). Exiting...\n",
                        args.outfq1, n_reads_out);
              rc = EXIT_FAILURE;
              goto free_and_exit;
         }

         if (pe_mode) {
              if (0 >= gzprintf_fastq(fp_outfq2, seq2, trim_pos_2)) {
                   LOG_ERROR("Couldn't write to %s (after successfully"
                             " writing %d reads). %s\n",
                             args.outfq2, n_reads_out,
                             EARLY_EXIT_MESSAGE);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }

         n_reads_out+=1;
    }
    /* while len1 */

	if (pe_mode) {
         if ((len2 = kseq_read(seq2)) >= 0) {
              LOG_ERROR("Reached premature end in first file (%s)."
                        " Still received reads from second file (%s from %s). %s\n",
                        args.infq1, seq2->name.s, args.infq2, EARLY_EXIT_MESSAGE);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
    }

    rc = EXIT_SUCCESS;

free_and_exit:
    free(trim_pos_1);
    free(trim_pos_2);

    LOG_INFO("%d %s in. %d %s out\n", n_reads_in, pe_mode?"pairs":"reads", 
             n_reads_out, pe_mode?"pairs":"reads");

	kseq_destroy(seq1);
    gzclose(fp_infq1);
    gzclose(fp_outfq1);

    if (pe_mode) {
         kseq_destroy(seq2);
         gzclose(fp_infq2);
         gzclose(fp_outfq2);
    }
    free_args(& args);

    /* fclose(stdout); fclose(stderr); */
    return rc;
}
