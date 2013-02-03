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

#include <argtable2.h>
#include <zlib.h>

#include "kseq.h"
KSEQ_INIT(gzFile, gzread)


#define MYNAME "fastq_filter"
#define DEFAULT_MINQUAL 3
#define DEFAULT_MINLEN 60
#define DEFAULT_PHREDOFFSET 33
#define XSTR(a) STR(a)
#define STR(a) #a
/* prints macro argument as string */

typedef struct {
char *infq1;
char *infq2;
char *outfq1;
char *outfq2;
int minqual;
int phredoffset;
int minlen;
} args_t;


/* Logging macros. You have to use at least one fmt+string. Newline
 * characters will not be appended automatically
 */
int debug = 0;
int verbose = 1;
/* print only if debug is true*/
#define LOG_DEBUG(fmt, args...)     printk(stderr, debug, "DEBUG(%s|%s): " fmt, __FILE__, __FUNCTION__, ## args)
/* print only if verbose is true*/
#define LOG_VERBOSE(fmt, args...)   printk(stderr, verbose, fmt, ## args)
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



int file_exists(char *fname) 
{
  /* from 
   * http://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform 
   */
  if (access(fname, F_OK) != -1) {
      return 1;
  } else {
      return 0;
  }
}


void dump_args(args_t *args) 
{
     LOG_DEBUG("args:\n");
     LOG_DEBUG("  infq1       = %s\n", args->infq1);
     LOG_DEBUG("  infq2       = %s\n", args->infq2);
     LOG_DEBUG("  outfq1      = %s\n", args->outfq1);
     LOG_DEBUG("  outfq2      = %s\n", args->outfq2);
     LOG_DEBUG("  minqual     = %d\n", args->minqual);
     LOG_DEBUG("  phredoffset = %d\n", args->phredoffset);
     LOG_DEBUG("  minlen      = %d\n", args->minlen);
}

void free_args(args_t *args)
{
     free(args->infq1);
     free(args->infq2);
     free(args->outfq1);
     free(args->outfq2);
}


/* will parse command line arguments and set members in args
 * accodingly. returns -1 on error (logic errors etc). caller has to
 * free args members using free_args(), even if -1 was returned
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
     struct arg_int *opt_minqual = arg_int0(
          "q", "minqual", "<int>",
          "Trim from end if base-call quality below this value."
          " Default: " XSTR(DEFAULT_MINQUAL));
     struct arg_int *opt_phredoffset = arg_int0(
          "Q", "phred", "<33|64>",
          "Qualities are ASCII-encoded Phred +33 (e.g. Sanger, SRA,"
          " Illumina 1.8+) or +64 (e.g. Illumina 1.3-1.7)."
          " Default: " XSTR(DEFAULT_PHREDOFFSET));
     struct arg_int *opt_minlen = arg_int0(
          "l", "minlen", "<int>",
          "Discard reads if below this length (discard both reads if"
          " either is below this limit). Default: " XSTR(DEFAULT_MINLEN));
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
     opt_minlen->ival[0] = DEFAULT_MINLEN;
     opt_minqual->ival[0] = DEFAULT_MINQUAL;
     opt_phredoffset->ival[0] = DEFAULT_PHREDOFFSET;
     
     void *argtable[] = {opt_infq1, opt_infq2, opt_outfq1, opt_outfq2,
                         opt_minqual, opt_minlen, opt_phredoffset,
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
          /* FIXME argtable bug? segfault if --i1 without arg
             fprintf(stderr, "opt_infq2->count=%d opt_infq2->filename[0]=%s\n", opt_infq2->count, opt_infq2->filename[0]);
          */
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

     args->minqual = opt_minqual->ival[0];
     if (args->minqual<0) {
          LOG_ERROR("Invalid quality '%d'\n", args->minqual);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }

     args->phredoffset = opt_phredoffset->ival[0];
     if (33 != args->phredoffset && 64 != args->phredoffset) {
          LOG_ERROR("Invalid Phred-quality ASCII offset '%d'\n", args->phredoffset);
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }

     
     args->minlen = opt_minlen->ival[0];
     if (args->minlen<1) {
          LOG_ERROR("Invalid length '%d'\n", args->minlen);          
          arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
          return -1;            
     }
     
     verbose = opt_verbose->count;
     debug = opt_debug->count;
     if (debug) {
          verbose=1;
     }
     
     arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
     return 0;
}



static inline
int gzprintf_fastq(gzFile fp, kseq_t *s) {
     return gzprintf(fp, "@%s\n%s\n+\n%s\n",
                     s->name.s, s->seq.s, s->qual.s);
}



int main(int argc, char *argv[])
{
    args_t args = { 0 };
    gzFile fp_infq1 = NULL, fp_infq2 = NULL;
    gzFile fp_outfq1 = NULL, fp_outfq2 = NULL;
    kseq_t *seq1 = NULL, *seq2 = NULL;
    int len1 = -1, len2 = -1;
    int paired_end = 0; /* bool paired end mode */
    int n_reads_in = 0, n_reads_out = 0; /* number of reads or pairs */
    int rc = EXIT_SUCCESS;

    if (parse_args(&args, argc, argv)) {
         return EXIT_FAILURE;
    }
    if (debug) {
         dump_args(& args);
    }

    if (args.infq2) {
         paired_end = 1;
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

	if (paired_end) {
         fp_infq2 = gzopen(args.infq2, "r");
         fp_outfq2 = gzopen(args.outfq2, "w");
         if (NULL == fp_infq2 || NULL == fp_outfq2) {
              LOG_ERROR("%s\n", "Couldn't open files");
              free_args(& args);
              return EXIT_FAILURE;
         }
    }

	seq1 = kseq_init(fp_infq1);
	if (paired_end) {
         seq2 = kseq_init(fp_infq2);
    }
    n_reads_in = n_reads_out = 0;
	while ((len1 = kseq_read(seq1)) >= 0) {
         if (paired_end) {
              if ((len2 = kseq_read(seq2)) < 0) {
                   LOG_ERROR("%s\n", "Reached premature end in second file. Still received reads from first file.");
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }
         n_reads_in+=1;



         LOG_FIXME("%s\n", "filtering goes here. Note: encoding issues!");
         if (n_reads_in==1) {
              int i;
              for (i=0; i<seq1->qual.l; i++) {
                   LOG_WARN("%d: %c %d\n", i, seq1->qual.s[i], seq1->qual.s[i]-args.phredoffset);
              }
         }
         exit(1);

         if (0 >= gzprintf_fastq(fp_outfq1, seq1)) {
              LOG_ERROR("Couldn't write to %s (after successfully writing %d reads).\n",
                        args.outfq1, n_reads_out);
              rc = EXIT_FAILURE;
              goto free_and_exit;
         }

         if (paired_end) {
              if (0 >= gzprintf_fastq(fp_outfq2, seq2)) {
                   LOG_ERROR("Couldn't write to %s (after successfully writing %d reads).\n",
                             args.outfq2, n_reads_out);
                   rc = EXIT_FAILURE;
                   goto free_and_exit;
              }
         }
         n_reads_out+=1;
	}
	if (paired_end) {
         LOG_FIXME("%s\n", "FIXME: check if second file if != NULL still has seqs and warn");
    }

free_and_exit:

	kseq_destroy(seq1);
	gzclose(fp_infq1);
	gzclose(fp_outfq1);
    if (paired_end) {
         kseq_destroy(seq2);
         gzclose(fp_infq2);
         gzclose(fp_outfq2);
    }
    free_args(& args);

    LOG_FIXME("%s\n", "Add tests");

	return rc;
}
