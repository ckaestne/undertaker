/*------------------------------------------------------------------------*/

#include "config.h"

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/*------------------------------------------------------------------------*/

#include "limmat.h"

/*------------------------------------------------------------------------*/
/* These values are according to the requirements of the SAT'2002 sat solver
 * competition.
 */
#define SATISFIABLE_EXIT_VALUE 10
#define UNSATISFIABLE_EXIT_VALUE 20
#define ABNORMAL_EXIT_VALUE 0
#define EXHAUSTED_EXIT_VALUE 0

/*------------------------------------------------------------------------*/

#define USAGEFMT \
"usage: limmat [<option> ...] [ <file> ... ]\n" \
"\n" \
"where <option> is one of the following options\n" \
"\n" \
"  -s | --sparse                print only result, time and memory\n" \
"  -h | --help                  print this command line summary\n" \
"  -p | --pretty-print          parse and print only\n" \
"  -v | --verbose               increase verbose level\n" \
"  -t<sec> | --time-out=<sec>   specify timeout\n" \
"  -m<m> | --max-decisions=<m>  specify maximal number of decisions\n" \
"  --clauses                    print assignment as list of clauses\n" \
"  --version                    print version and exit\n" \
"  --copyright                  print copyright and exit\n" \
"\n" \
"If <file> which should contain a CNF formula in DIMACS format is omitted\n" \
"then <stdin> is used for reading.  Specifying the verbose flag will\n" \
"automatically disable sparse mode.  Sparse output is disabled by default\n" \
"if only one input file is given.  For multiple input files it is enabled\n" \
"by default.\n"

static void
usage (void)
{
  printf (USAGEFMT);
}

/*------------------------------------------------------------------------*/

static Limmat *limmat = 0;
static int verbose = 0, sparse = 0, pretty_print = 0, multiple_files = 0;
static char *prefix = 0, *suffix = 0;
static int len_prefix, len_suffix;
static int assignment_as_clauses = 0;

/*------------------------------------------------------------------------*/

static int
skip_arg (char **argv, int *i)
{
  int ch, res;

  if (argv[*i][0] == '-')
    {
      if ((ch = argv[*i][1]) == 'm' || ch == 't')
	*i += 1;

      res = 1;
    }
  else
    res = 0;

  return res;
}

/*------------------------------------------------------------------------*/

static char *
common_string (char **argv, int argc, char seperator)
{
  char *res, *p, *q, *last_seperator_start;
  int i;

  res = 0;

  for (i = 1; i < argc; i++)
    {
      if (!skip_arg (argv, &i))
	{
	  if (res)
	    {
	      last_seperator_start = res - 1;
	      for (p = argv[i], q = res; *p && *p == *q; p++, q++)
		{
		  if (*q == seperator)
		    last_seperator_start = q;
		}

	      if (*q)
		last_seperator_start[1] = 0;
	    }
	  else
	    res = strdup (argv[i]);
	}
    }

  return res;
}

/*------------------------------------------------------------------------*/

static char *
reverse_string (const char *str)
{
  char *res, *q;
  const char *p;
  int len;

  len = strlen (str);
  res = (char *) malloc (len + 1);
  res[len] = 0;

  for (p = str, q = res + len - 1; *p; p++, q--)
    *q = *p;

  return res;
}

/*------------------------------------------------------------------------*/

static char *
common_prefix (char **argv, int argc)
{
  return common_string (argv, argc, '/');
}

/*------------------------------------------------------------------------*/

static char *
common_suffix (char **argv, int argc)
{
  char **my_argv, *res;
  int i, my_argc;

  my_argv = (char **) malloc (sizeof (char *) * argc);

  for (i = 1, my_argc = 1; i < argc; i++)
    {
      if (!skip_arg (argv, &i))
	my_argv[my_argc++] = reverse_string (argv[i]);
    }

  res = common_string (my_argv, my_argc, '.');

  for (i = 1; i < my_argc; i++)
    free (my_argv[i]);

  free (my_argv);

  return res;
}

/*------------------------------------------------------------------------*/

static void
catch_signal (int sid)
{
  FILE *file;
  int res;

  if (sparse)
    file = stdout;
  else
    {
#ifdef SAT2002FMT
      file = stdout;
#else
      file = stderr;
#endif
      fputs (LIMMAT_PREFIX "*** ", file);
    }

  switch (sid)
    {
    case SIGINT:
      res = ABNORMAL_EXIT_VALUE;
      fprintf (file, "interrupt");
      break;

    case SIGALRM:
      res = EXHAUSTED_EXIT_VALUE;
      fprintf (file, "time out");
      break;

    default:
      res = ABNORMAL_EXIT_VALUE;
      fprintf (file, "caught signal(%d)", sid);
      break;
    }

  if (sparse)
    fputc ('\n', file);
  else
#ifdef SAT2002FMT
    fputc ('\n', file);
#else
    fputs ("\n\n", file);
#endif

  fflush (file);

  if (limmat && verbose)
    {
      adjust_timer_Limmat (limmat);
      stats_Limmat (limmat, file);
      fflush (file);
    }

  exit (res);
}

/*------------------------------------------------------------------------*/

static void
parse (const char *name)
{
  if (!read_Limmat (limmat, 0, name))
    {
      fprintf (stderr, "%s\n", error_Limmat (limmat));
      exit (1);
    }
}

/*------------------------------------------------------------------------*/

static void
report (const char *msg)
{
#ifdef SAT2002FMT
  printf ("s ");
#endif
  printf ("%s\n", msg);
}

/*------------------------------------------------------------------------*/

static void
child (const char *name, int time_out, int max_decisions)
{
  const int *assignment, *p;
  char *short_name;
  int bytes, res;
  FILE *log;

#ifdef SAT2002FMT
  log = stdout;
#else
  log = stderr;
#endif

  (void) signal (SIGINT, catch_signal);
  (void) signal (SIGALRM, catch_signal);

  alarm (time_out);
  limmat = new_Limmat (verbose ? log : 0);

  if (verbose)
    {
      fprintf (log,
	       LIMMAT_PREFIX
	       "Limmat Satisfiability Solver Version %s\n"
	       LIMMAT_PREFIX "%s\n"
	       LIMMAT_PREFIX "%s\n",
	       version_Limmat (), copyright_Limmat (), id_Limmat ());

      strategy_Limmat (limmat, log);

      fprintf (log,
	       LIMMAT_PREFIX
	       "reading DIMACS file %s\n", name ? name : "<stdin>");
    }

  if (sparse)
    {
      if (multiple_files)
	{
	  assert (prefix);
	  assert (suffix);
	  short_name = strdup (name + len_prefix);
	  short_name[strlen (short_name) - len_suffix] = 0;
	}
      else
	short_name = strdup (name ? name : "<stdin>");

      printf ("%s ", short_name);
      fflush (stdout);
      free (short_name);
    }

  parse (name);

  if (verbose)
    {
      fprintf (log,
	       LIMMAT_PREFIX
	       "parsed %d clauses with %d literals in %.2f seconds\n",
	       clauses_Limmat (limmat),
	       literals_Limmat (limmat), time_Limmat (limmat));
      fflush (log);
    }

  res = sat_Limmat (limmat, max_decisions);
  alarm (24 * 3600);		/* disable alarms */

  if (sparse)
    {
      printf ("%.2f seconds ", time_Limmat (limmat));

      bytes = bytes_Limmat (limmat);
      if (bytes > (1 << 30))
	printf ("%.1f GB", bytes / (double) (1 << 30));
      else if (bytes > (1 << 20))
	printf ("%.1f MB", bytes / (double) (1 << 20));
      else if (bytes > (1 << 10))
	printf ("%.1f KB", bytes / (double) (1 << 10));
      else
	printf ("%d bytes", bytes);

      fputc (' ', stdout);
    }

  switch (res)
    {
    case 0:
      report ("UNSATISFIABLE");
      res = UNSATISFIABLE_EXIT_VALUE;
      break;

    case 1:
      if (sparse || !assignment_as_clauses)
	report ("SATISFIABLE");

      if (!sparse)
	{
	  assignment = assignment_Limmat (limmat);
	  assert (assignment);

	  if (assignment_as_clauses)
	    {
	      for (p = assignment; *p; p++)
		printf ("%d 0\n", *p);
	    }
	  else
	    {
	      print_assignment_Limmat (assignment, stdout);
#ifdef SAT2002FMT
	      printf ("v 0\n");
#endif
	    }
	}

      res = SATISFIABLE_EXIT_VALUE;
      break;

    default:
      assert (res == -1);
      report ("DECISION");
      res = EXHAUSTED_EXIT_VALUE;
      break;
    }

  if (verbose)
    stats_Limmat (limmat, log);
  delete_Limmat (limmat);

  exit (res);
}

/*------------------------------------------------------------------------*/

int
main (int argc, char **argv)
{
  int i, pid, status, res;
  int time_out, max_decisions;
  char *name;

  pretty_print = 0;
  multiple_files = 0;
  time_out = 3600;
  max_decisions = -1;		/* no bound is default */
#ifdef SAT2002FMT
  verbose = 1;
#else
  verbose = 0;
#endif
  limmat = 0;
  sparse = 0;
  name = 0;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
	{
	  usage ();
	  exit (0);
	}
      else if (!strcmp (argv[i], "--clauses"))
	{
	  assignment_as_clauses = 1;
	}
      else if (!strcmp (argv[i], "--version"))
	{
	  printf ("%s\n", version_Limmat ());
	  exit (0);
	}
      else if (!strcmp (argv[i], "--options"))
	{
	  printf ("%s\n", options_Limmat ());
	  exit (0);
	}
      else if (!strcmp (argv[i], "--copyright"))
	{
	  printf ("%s\n", copyright_Limmat ());
	  exit (0);
	}
      else if (!strcmp (argv[i], "-v") || !strcmp (argv[i], "--verbose"))
	{
	  verbose++;
	}
      else if (!strcmp (argv[i], "-s") || !strcmp (argv[i], "--sparse"))
	{
	  sparse++;
	}
      else if (!strcmp (argv[i], "-p") || !strcmp (argv[i], "--pretty-print"))
	{
	  pretty_print = 1;
	}
      else if (argv[i][0] == '-' && argv[i][1] == 'm')
	{
	  if (argv[i][2])
	    {
	      if (isdigit ((int) argv[i][2]))
		{
		  max_decisions = atoi (argv[i] + 2);
		}
	      else
		goto EXPECTED_DIGIT_FOR_MAX_DECISIONS_ARGUMENT;
	    }
	  else if (++i < argc && isdigit ((int) argv[i][0]))
	    {
	      max_decisions = atoi (argv[i]);
	    }
	  else
	    goto EXPECTED_DIGIT_FOR_MAX_DECISIONS_ARGUMENT;
	}
      else
	if (argv[i][0] == '-' &&
	    argv[i][1] == '-' &&
	    argv[i][2] == 't' &&
	    argv[i][3] == 'i' &&
	    argv[i][4] == 'm' &&
	    argv[i][5] == 'e' &&
	    argv[i][6] == '-' &&
	    argv[i][7] == 'o' &&
	    argv[i][8] == 'u' && argv[i][9] == 't' && argv[i][10] == '=')
	{
	  if (isdigit ((int) argv[i][11]))
	    {
	      max_decisions = atoi (argv[i] + 11);
	    }
	  else
	    {
	    EXPECTED_DIGIT_FOR_MAX_DECISIONS_ARGUMENT:
	      fprintf (stderr,
		       "*** limmat: expected digit for max decisions argument\n");
	      exit (1);
	    }
	}
      else if (argv[i][0] == '-' && argv[i][1] == 't')
	{
	  if (argv[i][2])
	    {
	      if (isdigit ((int) argv[i][2]))
		{
		  time_out = atoi (argv[i] + 2);
		}
	      else
		goto EXPECTED_DIGIT_FOR_TIME_OUT_ARGUMENT;
	    }
	  else if (++i < argc && isdigit ((int) argv[i][0]))
	    {
	      time_out = atoi (argv[i]);
	    }
	  else
	    goto EXPECTED_DIGIT_FOR_TIME_OUT_ARGUMENT;
	}
      else
	if (argv[i][0] == '-' &&
	    argv[i][1] == '-' &&
	    argv[i][2] == 't' &&
	    argv[i][3] == 'i' &&
	    argv[i][4] == 'm' &&
	    argv[i][5] == 'e' &&
	    argv[i][6] == '-' &&
	    argv[i][7] == 'o' &&
	    argv[i][8] == 'u' && argv[i][9] == 't' && argv[i][10] == '=')
	{
	  if (isdigit ((int) argv[i][11]))
	    {
	      time_out = atoi (argv[i] + 11);
	    }
	  else
	    {
	    EXPECTED_DIGIT_FOR_TIME_OUT_ARGUMENT:
	      fprintf (stderr,
		       "*** limmat: expected digit for time out argument\n");
	      exit (1);
	    }
	}
      else if (argv[i][0] == '-')
	{
	  fprintf (stderr,
		   "*** limmat: unknown command line option '%s' (try '-h')\n",
		   argv[i]);
	  exit (1);
	}
      else if (name)
	{
	  multiple_files = 1;
	}
      else
	name = argv[i];
    }

  if (multiple_files)
    {
      prefix = common_prefix (argv, argc);
      suffix = common_suffix (argv, argc);
      len_prefix = strlen (prefix);
      len_suffix = strlen (suffix);
      sparse = 1;
    }

  if (verbose)
    sparse = 0;

  if (pretty_print)
    {
      if (multiple_files)
	{
	  fprintf (stderr,
		   "*** limmat: can not pretty print multiple files\n");
	  exit (1);
	}

      limmat = new_Limmat (0);
      parse (name);
      print_Limmat (limmat, stdout);
      delete_Limmat (limmat);
      res = ABNORMAL_EXIT_VALUE;
    }
  else
    {
      res = 0;

      if (multiple_files)
	{
	  for (i = 1; !res && i < argc; i++)
	    {
	      if (!skip_arg (argv, &i))
		{
		  if ((pid = fork ()))
		    {
		      wait (&status);
		      if (WIFSIGNALED (status) ||
			  WEXITSTATUS (status) == ABNORMAL_EXIT_VALUE)
			{
			  res = ABNORMAL_EXIT_VALUE;
			}
		    }
		  else
		    child (argv[i], time_out, max_decisions);
		}
	    }
	}
      else
	child (name, time_out, max_decisions);
    }

  if (prefix)
    free (prefix);

  if (suffix)
    free (suffix);

  exit (res);
  return res;
}
