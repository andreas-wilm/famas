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

#include <argtable2.h>
#include <zlib.h>

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)


#define MYNAME "fastq_filter"
#define DEFAULT_MIN3PQUAL 3
#define DEFAULT_MIN5PQUAL 0
#define DEFAULT_MINREADLEN 60
#define DEFAULT_PHREDOFFSET 33
#define XSTR(a) STR(a)
#define STR(a) #a
/* prints macro argument as string */

typedef struct {
     char *infq1;
     char *infq2;
     char *outfq1;
     char *outfq2;
     int min5pqual;
     int min3pqual;
     int phredoffset;
     int minreadlen;
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
int debug = 0;
int verbose = 1;
/* print only if debug is true*/
#define LOG_DEBUG(fmt, args...)     printk(stderr, debug, "DEBUG(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args)
/* print only if verbose is true*/
#define LOG_INFO(fmt, args...)   printk(stderr, verbose, fmt, ## args)
/* always warn to stderr */
#define LOG_WARN(fmt, args...)      printk(stderr, 1, "WARNING(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args)
/* always print errors to stderr*/
#define LOG_ERROR(fmt, args...)     printk(stderr, 1, "ERROR(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args)
/* always print fixme's */
#define LOG_FIXME(fmt, args...)     printk(stderr, 1, "FIXME(%s|%s:%d): " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)

/* Taken from the Linux kernel source and slightly modified.
 * bool_flag: print or don't
 */
int
printk(FILE *stream, int bool_flag, const char *fmt, ...)
{                
    va_list args;
    static char printk_buf[8192];
    int printed_len=0;
 
    if (bool_flag) {
        /* Emit the output into the temporary buffer */
        va_start(args, fmt);
        printed_len = vsnprintf(printk_buf, sizeof(printk_buf), fmt, args);
        va_end(args);

        fprintf(stream, "%s", printk_buf);
        fflush(stream);        
    }
    return printed_len;
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
     LOG_DEBUG("args:\n");
     LOG_DEBUG("  infq1       = %s\n", args->infq1);
     LOG_DEBUG("  infq2       = %s\n", args->infq2);
     LOG_DEBUG("  outfq1      = %s\n", args->outfq1);
     LOG_DEBUG("  outfq2      = %s\n", args->outfq2);
     LOG_DEBUG("  min5pqual     = %d\n", args->min5pqual);
     LOG_DEBUG("  min3pqual     = %d\n", args->min3pqual);
     LOG_DEBUG("  phredoffset = %d\n", args->phredoffset);
     LOG_DEBUG("  minreadlen      = %d\n", args->minreadlen);
}


void free_args(args_t *args)
{
     free(args->infq1);
     free(args->infq2);
     free(args->outfq1);
     free(args->outfq2);
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
     struct arg_int *opt_min5pqual = arg_int0(
          "Q", "min5pqual", "<int>",
          "Trim from start/5'-end if base-call quality below this value."
          " Default: " XSTR(DEFAULT_MIN5PQUAL));
     struct arg_int *opt_min3pqual = arg_int0(
          "q", "min3pqual", "<int>",
          "Trim from end/3'-end if base-call quality below this value."
          " Default: " XSTR(DEFAULT_MINQUAL));
     struct arg_int *opt_phredoffset = arg_int0(
          "e", "phred", "<33|64>",
          "Qualities are ASCII-encoded Phred +33 (e.g. Sanger, SRA,"
          " Illumina 1.8+) or +64 (e.g. Illumina 1.3-1.7)."
          " Default: " XSTR(DEFAULT_PHREDOFFSET));
     struct arg_int *opt_minreadlen = arg_int0(
          "l", "minlen", "<int>",
          "Discard reads if below this length (discard both reads if"
          " either is below this limit). Default: " XSTR(DEFAULT_MINREADLEN));
     struct arg_lit *opt_help = arg_lit0(
          "h", "help",
          "Print this help and exit");
     struct arg_lit *opt_verbose  = arg_lit0(
          "v", "verbose", 
          "Verbose output to stderr");
     struct arg_lit *opt_debug  = arg_lit0(
          NULL, "debug", 
          "Print debugging info");
     struct arg_end *opt_end = arg_end(10); /* maximum number of errors
                                             * to store */
     
     /* set defaults */
     opt_minreadlen->ival[0] = DEFAULT_MINREADLEN;
     opt_min5pqual->ival[0] = DEFAULT_MIN5PQUAL;
     opt_min3pqual->ival[0] = DEFAULT_MIN3PQUAL;
     opt_phredoffset->ival[0] = DEFAULT_PHREDOFFSET;
     
     void *argtable[] = {opt_infq1, opt_infq2, opt_outfq1, opt_outfq2,
                         opt_min5pqual, opt_min3pqual, opt_minreadlen, opt_phredoffset,
                         opt_help, opt_verbose, opt_debug,
                         opt_end};    
     
     if (arg_nullcheck(argtable)) {
          LOG_ERROR("%s\n", "insufficient memory for argtable allocation");
          return -1;
     }

     nerrors = arg_parse(argc, argv, argtable);

     /* Special case: '--help' takes precedence over error reporting
      * and we are allowed to exit from here */
     if (opt_help->count > 0) {
          fprintf(stderr, "Usage: %s\n", MYNAME);
          arg_print_syntax(stdout, argtable, "\n");
          fprintf(stderr, "\nThis is yet another program for FastQ filtering\n");
          arg_print_glossary(stdout, argtable, "  %-25s %s\n");
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          exit(0);
     }
     
     if (nerrors > 0) {
          arg_print_errors(stdout, opt_end, MYNAME);
          fprintf(stderr, "For more help try: %s -h or --help\n", MYNAME);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;
     }
     
     args->infq1 = strdup(opt_infq1->filename[0]);
     if (0 != strncmp(args->infq1, "-", 1) && ! file_exists(args->infq1)) {
          LOG_ERROR("File %s does not exist\n", args->infq1);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;
     }

     args->outfq1 = strdup(opt_outfq1->filename[0]);
     if (0 != strncmp(args->outfq1, "-", 1) && file_exists(args->outfq1)) {
          LOG_ERROR("Cowardly refusing to overwrite existing file %s\n", args->outfq1);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;
     }

     if (opt_infq2->count) {
          args->infq2 = strdup(opt_infq2->filename[0]);
          if (0 == strcmp(args->infq2, args->infq1)) {
               LOG_ERROR("%s\n", "The two input fastq files are the same file");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return -1;              
          }
          if (! file_exists(args->infq2)) {
               LOG_ERROR("File %s does not exist\n", args->infq2);
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return -1;
          }
          
          if (! opt_outfq2->count) {
               LOG_ERROR("%s\n", "Need two output files for paired-end input");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return -1;                        
          }

          args->outfq2 = strdup(opt_outfq2->filename[0]);
          if (file_exists(args->outfq2)) {
               LOG_ERROR("Cowardly refusing to overwrite existing file %s\n", args->outfq2);
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return -1;
          }
     } else {
          if (opt_outfq2->count) {
               LOG_ERROR("%s\n", "Got second output file, not a corresponding second input file");
               arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
               return -1;              
          }
     }

     args->min5pqual = opt_min5pqual->ival[0];
     if (args->min5pqual<0) {
          LOG_ERROR("Invalid quality '%d'\n", args->min5pqual);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }

     args->min3pqual = opt_min3pqual->ival[0];
     if (args->min3pqual<0) {
          LOG_ERROR("Invalid quality '%d'\n", args->min3pqual);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }

     args->phredoffset = opt_phredoffset->ival[0];
     if (33 != args->phredoffset && 64 != args->phredoffset) {
          LOG_ERROR("Invalid Phred-quality ASCII offset '%d'\n", args->phredoffset);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }

     
     args->minreadlen = opt_minreadlen->ival[0];
     if (args->minreadlen<1) {
          LOG_ERROR("Invalid length '%d'\n", args->minreadlen);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }
     
     verbose = opt_verbose->count;
     debug = opt_debug->count;
     if (debug) {
          verbose=1;
     }
     
     /* Whew! */

     arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
     return 0;
}


/* returns -1 if read is to be discarded. otherwise trim_pos will hold
 * valid trimming positions (zero-offset)
 */
int calc_trim_pos(trim_pos_t *trim_pos, 
                  const kseq_t *seq, const int phredoffset,
                  const trim_args_t *trim_args)
{
     int i;

     assert(trim_args->minreadlen > 0);

     trim_pos->pos5p = -1;
     trim_pos->pos3p = -1;

     if (trim_args->minreadlen > seq->qual.l) {
          return -1;
     }

     /* 5p end */
     for (i=0; i<seq->qual.l - trim_args->minreadlen; i++) {
          int q = seq->qual.s[i] - phredoffset;
          /*LOG_DEBUG("Checking 5p pos %d with q %d\n", i, q);*/
          if (q >= trim_args->min5pqual) {
               trim_pos->pos5p = i;
               break;
          }
     }
     if (trim_pos->pos5p == -1) {
          return -1;
     }

     /* 3p end */
     for (i=seq->qual.l-1; i >= trim_args->minreadlen-1; i--) {/* FIXME max min_read_len trim_pos_5p? */
          int q = seq->qual.s[i]-phredoffset;
          /*LOG_DEBUG("Checking 3p pos %d with q %d\n", i, q);*/
          if (q >= trim_args->min3pqual) {
               trim_pos->pos3p = i;
               break;
          }
     }
     if (trim_pos->pos3p == -1) {
          return -1;
     }
#if 0
      LOG_FIXME("trim_pos_5p = %d\n", trim_pos->pos5p);
      LOG_FIXME("trim_pos_3p = %d\n", trim_pos->pos3p);

      for (i=0; i<seq->qual.l; i++) {
           LOG_WARN("%d: %c %d\n", i, seq->qual.s[i], seq->qual.s[i]-phredoffset);
      }
#endif
      
      if (trim_pos->pos3p - trim_pos->pos5p + 1 < trim_args->minreadlen) {
           return -1;
      }
      
      return 0;
}


int gzprintf_fastq(gzFile fp, kseq_t *s) {
     int ret;
     ret = gzprintf(fp, "@%s", s->name.s);
     if (s->comment.l) {
         ret += gzprintf(fp, " %s", s->comment.s);
     }
     ret += gzprintf(fp, "\n%s\n+\n%s\n", s->seq.s, s->qual.s);
     return ret;
 }


int main(int argc, char *argv[])
{
    args_t args = { 0 };
    gzFile fp_infq1 = NULL, fp_infq2 = NULL;
    gzFile fp_outfq1 = NULL, fp_outfq2 = NULL;
    kseq_t *seq1 = NULL, *seq2 = NULL;
    int len1 = -1, len2 = -1;
    int paired = 0; /* bool paired end mode */
    int n_reads_in = 0, n_reads_out = 0; /* number of reads or pairs */
    trim_args_t trim_args;
    int rc = EXIT_SUCCESS;

    if (parse_args(&args, argc, argv)) {
         free_args(& args);
         return EXIT_FAILURE;
    }
    if (debug) {
         dump_args(& args);
    }

    if (args.infq2) {
         paired = 1;
    }

    fp_infq1 = (0 == strcmp(args.infq1, "-")) ? 
         gzdopen(fileno(stdin), "r") : gzopen(args.infq1, "r");
    fp_outfq1 = (0 == strcmp(args.outfq1, "-")) ? 
         gzdopen(fileno(stdout), "w") : gzopen(args.outfq1, "w");
    if (NULL == fp_infq1 || NULL == fp_outfq1) {
         LOG_ERROR("%s\n", "Couldn't open files.");
         free_args(& args);
         return EXIT_FAILURE;
    }

	if (paired) {
         fp_infq2 = gzopen(args.infq2, "r");
         fp_outfq2 = gzopen(args.outfq2, "w");
         if (NULL == fp_infq2 || NULL == fp_outfq2) {
              LOG_ERROR("%s\n", "Couldn't open files");
              free_args(& args);
              return EXIT_FAILURE;
         }
    }



    trim_args.min5pqual = args.min5pqual;
    trim_args.min3pqual = args.min3pqual;;
    trim_args.minreadlen = args.minreadlen;

	seq1 = kseq_init(fp_infq1);
	if (paired) {
         seq2 = kseq_init(fp_infq2);
    }
    n_reads_in = n_reads_out = 0;
	while ((len1 = kseq_read(seq1)) >= 0) {
         int discard = 0;
         trim_pos_t trim_pos;

         if (paired) {
              if ((len2 = kseq_read(seq2)) < 0) {
                   LOG_ERROR("%s\n", "Reached premature end in second file. Still received reads from first file.");
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }
         n_reads_in+=1;


         if (0 != calc_trim_pos(&trim_pos, seq1, args.phredoffset, &trim_args)) {
              discard = 1;
         }
         LOG_FIXME("%s\n", "actual filtering missing");

         if (discard) {
              LOG_FIXME("Discarding %s\n", seq1->name.s);
              continue;
         }

         if (0 >= gzprintf_fastq(fp_outfq1, seq1)) {
              LOG_ERROR("Couldn't write to %s (after successfully writing %d reads).\n",
                        args.outfq1, n_reads_out);
              rc = EXIT_FAILURE;
              goto free_and_exit;
         }

         if (paired) {
              /* FIXME add id check as in pairedend_read_order.sh ? */
              if (0 >= gzprintf_fastq(fp_outfq2, seq2)) {
                   LOG_ERROR("Couldn't write to %s (after successfully writing %d reads).\n",
                             args.outfq2, n_reads_out);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }
         n_reads_out+=1;
	}
	if (paired) {
         LOG_FIXME("%s\n", "FIXME: check if second file if != NULL still has seqs and warn");
    }

free_and_exit:

    LOG_INFO("%d %s in. %d %s out\n", n_reads_in, paired?"pairs":"reads", 
             n_reads_out, paired?"pairs":"reads");

	kseq_destroy(seq1);
	gzclose(fp_infq1);
	gzclose(fp_outfq1);
    if (paired) {
         kseq_destroy(seq2);
         gzclose(fp_infq2);
         gzclose(fp_outfq2);
    }
    free_args(& args);

    LOG_FIXME("%s\n", "Add tests");

	return rc;
}
