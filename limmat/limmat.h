#ifndef _limmat_h_INCLUDED
#define _limmat_h_INCLUDED

/*------------------------------------------------------------------------*/
/* In case the user program wants to print additional verbose output, it
 * should use this prefix.
 */
#ifdef SAT2002FMT
#define LIMMAT_PREFIX "c "
#else
#define LIMMAT_PREFIX "<limmat> "
#endif

/*------------------------------------------------------------------------*/

typedef struct Limmat Limmat;

/*------------------------------------------------------------------------*/
/* Constructor and destructor for our SAT checker.  If the 'log' is non zero
 * we use it for logging status reports during satisfiability solving.
 */
Limmat *new_Limmat (FILE * log);
void delete_Limmat (Limmat *);
void set_log_Limmat (Limmat *, FILE * log);

/*------------------------------------------------------------------------*/

const char *id_Limmat (void);	/* RCS Id */
const char *version_Limmat (void);
const char *copyright_Limmat (void);
const char *options_Limmat (void);

/*------------------------------------------------------------------------*/
/* After creation of a 'Limmat' you can read in a DIMACS file with
 * 'read_Limmat'.  The string given as third argument is supposed to be the
 * name of the input file.  It is used in error messages for parse errors.
 * In case the FILE argument is zero, the library tries to open the file
 * with this name.  The name may be zero as well.  In this case the file
 * name is not included in the error message.  The result is non zero if
 * reading was succesfull.  Otherwise '0' is returned and an error message
 * is produced, that can be extracted with 'error_Limmat'.
 */
int read_Limmat (Limmat *, FILE *, const char *);
const char *error_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* Add a zero terminated list of literals to limmat.
 */
void add_Limmat (Limmat *, const int *clause);

/*------------------------------------------------------------------------*/
/* Print the current clause data base to the given file in DIMACS format.
 */
void print_Limmat (Limmat *, FILE *);

/*------------------------------------------------------------------------*/
/* Print some statistics.  Call 'adjust_timer_Limmat' in case you want to
 * include the time spent in an interrupted call.
 */
void stats_Limmat (Limmat *, FILE *);

/*------------------------------------------------------------------------*/
/* Print the setting of the parameters.  They can all be controlled by
 * environment variables.
 */
void strategy_Limmat (Limmat *, FILE *);

/*------------------------------------------------------------------------*/
/* Get the number of clauses and literals.
 */
int maxvar_Limmat (Limmat *);
int literals_Limmat (Limmat *);
int clauses_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* In a signal handler called by an interrupt you may want to adjust the
 * time spent in the library before printing the statistics.  Otherwise the
 * time of the interrupted call is not included in the statistics.
 */
void adjust_timer_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* Get time spent in the library in seconds.  Call 'adjust_timer_Limmat' in
 * case you want to include the time spent in the library in an interrupted
 * call.
 */
double time_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* Get number of currently allocated bytes in the library.
 */
size_t bytes_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* Determine satisfiability:  'sat_Limmat' returns '0' if the stored clauses
 * are unsatisfiable.  If a time out or space out occured a negative value
 * is returned.  Otherwise '1' is returned and a satisfying assignment has
 * been found. The assignment can be extracted with 'assignment_Limmat' as
 * a zero terminated list of literals.
 *
 * If 'max_decision' is less than 0, the number of decisions is unbounded.
 * If 'max_decision' is zero, than only unit resolution on the toplevel is
 * performed.  In this case we still check whether all variables are already
 * assigned without deriving a conflict.
 */
int sat_Limmat (Limmat *, int max_decisions);
const int *assignment_Limmat (Limmat *);

/*------------------------------------------------------------------------*/
/* Pretty print an assignment, a sequence of integers terminated by zero, by
 * wrapping numbers at a 80 column margin.
 */
void print_assignment_Limmat (const int *, FILE *);

/*------------------------------------------------------------------------*/
#endif
