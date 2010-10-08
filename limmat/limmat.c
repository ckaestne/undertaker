/*------------------------------------------------------------------------*\
(C)2001-2002, Armin Biere, Computer Systems Institute, ETH Zurich.

All rights reserved. Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the following
conditions are met:

  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

    This product includes software developed by Armin Biere, Computer
    Systems Institute, ETH Zurich.

  4. Neither the name of the University nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.
   
THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\*------------------------------------------------------------------------*/

#define COPYRIGHT \
"(C)2001-2002 Armin Biere, Computer Systems Institute, ETH Zurich"

const char *
copyright_Limmat (void)
{
  return COPYRIGHT;
}

/*------------------------------------------------------------------------*/

const char *
id_Limmat (void)
{
  return "$Id: limmat.c,v 1.98.2.41 2002/11/26 09:32:59 biere Exp $";
}

/*------------------------------------------------------------------------*/

#include "config.h"

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

/*------------------------------------------------------------------------*/

#include "limmat.h"

/*------------------------------------------------------------------------*/

const char *
version_Limmat (void)
{
  return LIMMAT_VERSION;
}


/*------------------------------------------------------------------------*/
/* To add more dynamic checks to the program you should define
 * 'CHECK_CONSISTENCY'.  This results in a small increase in memory usage
 * and a somewhat large time penalty.  You may additionally also define
 * 'EXPENSIVE_CHECKS' to enable thorough but expensive checks.
 *
#define EXPENSIVE_CHECKS
#define CHECK_CONSISTENCY
 */

/*------------------------------------------------------------------------*/
/* You should not change anything in this section.
 */
#ifdef EXPENSIVE_CHECKS
#ifndef CHECK_CONSISTENCY
#define CHECK_CONSISTENCY
#endif
#warning "#define EXPENSIVE_CHECKS"
#endif
#ifdef CHECK_CONSISTENCY
#warning "#define CHECK_CONSISTENCY"
#endif
/*------------------------------------------------------------------------*/
/* For easy debugging you may want to log the actions of the sat solving
 * engine.  This may produce a lot of output and should only be enabled for
 * debugging purposes.
 *
#define LOG_ASSIGNMENTS
#define LOG_BACKJUMP
#define LOG_BACKTRACK
#define LOG_CONFLICT
#define LOG_CONNECT
#define LOG_DECISION
#define LOG_LEARNED
#define LOG_PROPS
#define LOG_PUSH
#define LOG_RESCORE
#define LOG_RESTART
#define LOG_UIP
#define LOG_UNIT
 */

/*------------------------------------------------------------------------*/
#if defined(LOG_ASSIGNMENTS) || \
    defined(LOG_BACKTRACK) || \
    defined(LOG_CONFLICT) || \
    defined(LOG_DECISION) || \
    defined(LOG_LEARNED) || \
    defined(LOG_RESCORE) || \
    defined(LOG_RESTART) || \
    defined(LOG_UIP) || \
    defined(LOG_UNIT)
#warning "#define LOG_ ..."
#define LOGFILE(l) ((l)->log ? (l)->log : stdout)
#define LOG_SOMETHING
#endif
/*------------------------------------------------------------------------*/
#if defined(NO_INLINE_KEYWORD) || !defined(NDEBUG)
#define inline
#endif
/*------------------------------------------------------------------------*/
/* You may want to use different decision strategies.  These strategies can
 * still be set by environment variables.
 */
#define RESCORE 256		/* number of decisions before rescore */
#define RESCOREFACTOR ((double)0.5) /* score factor multiplied in rescore */
#define RESTART 10000		/* number of decisions before restart */
#define STATISTICS 1		/* enable statistics */

/*------------------------------------------------------------------------*/
/* The following section guarded by '!defined(EXTERNAL_DEFINES)' contains
 * all the critical compile time options that control the speed of the
 * solver.  You may define 'EXTERNAL_DEFINES' by using './configure
 * --external-defines', but then you have to compile the library manually
 * and specify all these options on the comman line.
 */
#if !defined(EXTERNAL_DEFINES)
/*------------------------------------------------------------------------*/
/* With a small runtime penalty and an additional restriction on the maximal
 * size of a stack, actually the maximal count of elements on the stack,
 * defining 'COMPACT_STACK' will use only two words for an inlined stack.
 * This may decrease the size of a variable structure considerably.
 *
 */
#define COMPACT_STACK

/*------------------------------------------------------------------------*/
/* Use the idea of Chaff to save the last search direction for each watched
 * literal.
 * 
 */
#define OPTIMIZE_DIRECTION

/*------------------------------------------------------------------------*/
/* Enable early detection of conflict literals in BCP.
 *
 */
#define EARLY_CONFLICT_DETECTION

/*------------------------------------------------------------------------*/
/* In case an already implied literal is again implied in BCP use the
 * shorter clause as reason for the implication.
 *
 */
#define SHORTEN_REASONS

/*------------------------------------------------------------------------*/
/* The Limmat data structure allows a constant lookup of the other watched
 * literal in the clause.  This may reduce the overall number of visits to
 * literals, since the other watched literal is often assigned to 1 already.
 *
 */
#define LOOK_AT_OTHER_WATCHED_LITERAL

/*------------------------------------------------------------------------*/
#else /* EXTERNAL DEFINES */
#if !defined(RESCORE) || !defined(RESTART) || !defined(STATISTICS)
#error "EXTERNAL_DEFINES requires to define RESCORE, RESTART, STATISTICS"
#endif
#endif
/*------------------------------------------------------------------------*/

#define VARIABLE_ALIGNMENT sizeof(void*)
#define ALL_BUT_SIGN_BITS 0
#define SIGN_BIT 1

typedef struct Clause *Occurence;

/*------------------------------------------------------------------------*/
/* Truth constants: By flipping their sign bit with 'not' they can be
 * converted to each other respectively.
 */
#define FALSE ((Variable*)0)
#define TRUE ((Variable*)SIGN_BIT)

/*------------------------------------------------------------------------*/
/* We use the lowest bit of (Variable*) and (Clause*) pointers for negation
 * and storing the link position respectively.  This bit stuffing technique
 * saves a lot of space but may be slower.  To manipulate the lowest bit of
 * pointers we have to cast a pointer to a machine word of the same size,
 * since we need to cast it back again without loosing information.
 *
 * On most machines 'unsigned long' has the same number of bytes as pointer.
 * This is checked in test case 'align0'.
 */
#define PTR_SIZED_WORD unsigned long

/*------------------------------------------------------------------------*/
/* Maximal size of a parse error string.
 */
#define ERROR_SIZE 200

/*------------------------------------------------------------------------*/
/* Memory management related constants for checking consistency.
 */
#ifdef CHECK_CONSISTENCY
#define DATA_OFFSET (((char*) &(((Bucket*)0)->aligned_data)) - (char*)0)
#endif

/*------------------------------------------------------------------------*/

typedef struct Arena Arena;
typedef struct Assignment Assignment;
typedef struct Clause Clause;
typedef struct Counter Counter;
typedef struct Frame Frame;
typedef struct Parser Parser;
typedef struct Stack Stack;
typedef struct Statistics Statistics;
typedef struct Variable Variable;

/*------------------------------------------------------------------------*/
#ifdef CHECK_CONSISTENCY
/*------------------------------------------------------------------------*/

typedef struct Bucket Bucket;

/*------------------------------------------------------------------------*/
/* A Bucket is a basic allocation unit.  It is only used in consistency
 * checking mode to save the allocation size together with allocated memory,
 * in order to check that the size given as parameter to 'delete' actually
 * is the same as the size given as parameter to 'new' or 'resize'.  Note
 * that aligning the data to 'double' may actually result on additional word
 * being inserted by the compiler after the 'size' field.  This may increase
 * the actual memory used by two words per allocation, e.g. 64 bytes on a 32
 * bit machine with 8 byte aligned 'double' fields.  If 'CHECK_CONSISTENCY'
 * is disabled no additional memory is required.
 */
struct Bucket
{
  size_t size;

  union
  {
    double as_double;
    void *as_pointer;
    int as_number;
  }
  aligned_data;
};

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* A generic stack implementation.  The data elements to be pushed on the
 * stack are generic 'void *' pointers.  Stacks are resized automatically,
 * but their size will never shrink during pop operations.  Since the stack
 * is implemented as contiguos area we provide a way to treat the stack
 * contents as an array of pointers.
 */
struct Stack
{
#ifdef COMPACT_STACK
#define LOG_LOG_MAX_STACK_SIZE 5
#define LOG_MAX_STACK_COUNT (sizeof(unsigned) * 8 - LOG_LOG_MAX_STACK_SIZE)
#define LOG_MAX_STACK_SIZE (1 << LOG_LOG_MAX_STACK_SIZE)
#define MAX_STACK_COUNT (1 << LOG_MAX_STACK_COUNT)
  unsigned internal_count:LOG_MAX_STACK_COUNT;
  unsigned log_size:LOG_LOG_MAX_STACK_SIZE;
#else
  int internal_count, internal_size;
#endif

  /* The stack is size optimized and avoids an additional heap allocation as
   * long the size is <= 1.
   */
  union
  {
    void **as_array;		/* if size > 1 */
    void *as_pointer;		/* size <= 1 */
  }
  data;
};

/*------------------------------------------------------------------------*/
/* An Arena is used for allocating aligned memory of aligned size.  It is
 * similar to a stack, since the new memory is always added to the end of
 * the already allocated memory.  Memory is kept contiguous thus that each
 * allocation may trigger resizing and moving the already allocated data.
 * This in turn requires that all pointers to allocated
 * memory in an arena may have to be fixed.
 */
struct Arena
{
  int alignment;
  char *mem_start, *mem_end, *data_start, *data_end;
};

/*------------------------------------------------------------------------*/

struct Variable
{
  Variable *assignment;		/* TRUE, FALSE, or 'this' */
  Occurence reason;		/* reason for the assignment */
  int decision_level;		/* the decision level if assigned */
  Stack watched[2];		/* watched occurences of this variable */
  int ref[2];			/* number of occurrences */

  int saved[2];			/* old score after last rescore */
  int score[2];			/* current score */
  int pos[2];			/* position in order */

  int id;			/* the external integer id of a variable */

  /* We use this 'mark' bit for graph traversal purposes.
   */
  unsigned mark:1;

  /* The 'sign_in_clause' is used to ensure that externally added clauses
   * contain each variable only once.  See 'unique_literals' for more
   * details.
   */
  unsigned sign_in_clause:1;

  /* If a variable is scheduled to be assigned, the position of the
   * scheduled assignment in the assignment queue is stored with the
   * variable.  This allows early detection and generation of conflicts, in
   * case a variable and its negation are both scheduled to be assigned to
   * true.
   */
  unsigned sign_on_queue:1;
  int pos_on_queue;
};

/*------------------------------------------------------------------------*/

struct Clause
{
  unsigned learned:1;		/* external or learned clause */
  int size:sizeof (unsigned) * 8 - 3;	/* number of literals */
#ifdef OPTIMIZE_DIRECTION
  int direction:2;
#endif
  int literals_idx;		/* points into literal stack */
  int id;
  int watched[2];		/* watched literals */
};

/*------------------------------------------------------------------------*/
/* The representation of a scheduled assignment is somewhat redundant, since
 * the decision level and also the literal can be extracted from the
 * reason, if 'reason' is non-zero.
 */
struct Assignment
{
  Variable *literal;
  int decision_level;
  Occurence reason;
};

/*------------------------------------------------------------------------*/
/* The control stack contains frames.  Each frame corresponds to a decision.
 * Since we do not necessarily flip each decision made, but use conflict
 * directed backtracking instead, there is no such notion as branches.
 */
struct Frame
{
  Variable *decision;		/* the decisoin assignment made */
  int decision_level;
  int trail_level;		/* the old level of the trail */
};

/*------------------------------------------------------------------------*/
/* A counter, that counts up to a certain threshold, and then starts again
 * with the threshold scaled by a certain factor.  We use it to control
 * rescore and restart operations and when to print a one line progress
 * report.
 */
struct Counter
{
  int current, init, inc;
  double factor, finished;
};

/*------------------------------------------------------------------------*/

struct Statistics
{
  double assignments, propagations, visits, uips, backjumps;
  double swapped, compared, searched;
  double learned_clauses, removed_clauses;
  double learned_literals, removed_literals;
  double rescored_variables, sum_assigned_in_decision, sum_score_in_decision;
  int original_clauses, max_clauses;
  int original_literals, max_literals;
};

/*------------------------------------------------------------------------*/

struct Limmat
{
  /* Control stack.
   */
  int control_size, decision_level;
  Frame *control;
  Stack trail;			/* assigned variables, ordered by decision level */

  /* Scheduled assignments queue.
   */
  int head_assignments, tail_assignments, size_assignments;
  Assignment *assignments;
  int inconsistent;		/* position of first inconsistent assignment */

  /* Variable manager.
   */
  int max_id, num_literals, num_variables, current_max_score, max_score;
  double added_literals;
  Arena variables;
  Stack order;			/* order variables for choosing decisions */

  /* Maintain interval in which we do not have to perform sorting of
   * variables in the 'order' and also do not have to search for new
   * assignments.
   */
  int last_assigned_pos;	/* position of last assigned variable */
  int first_zero_pos;		/* position of first zero score variable */

  /* Clause data base;
   */
  int contains_empty_clause;
  Stack clauses, units, literals;

  /* Necessary statistics.
   */
  double num_decisions, num_conflicts, added_clauses;
  int num_assigned, num_clauses;

  /* Additional statistics.
   */
  Statistics *stats;

  /* Save literals for generating conflict clauses during conflict analysis
   * on this stack.  Keeping a global stack for this purpose around saves us
   * some memory allocations.
   */
  Stack clause;

  /* A multipurpose stack used in inner functions onlys.
   */
  Stack stack;

  /* Logging.
   */
  FILE *log;

  /* Counters.
   */
  Counter report, restart, rescore;

  /* See 'RESCOREFACTOR'.
   */
  double score_factor;

  char *error;
  double time, timer;

  /* Memory manager.
   */
  size_t bytes, max_bytes;

#if defined(CHECK_CONSISTENCY) || defined(EXPENSIVE_CHECKS)
  int check_counter;
#endif

#ifdef EXPENSIVE_CHECKS
  int inner_mode;		/* no inner 'check_clause_is_implied' */
#endif

#ifdef LOG_SOMETHING
  int dont_log;			/* no internal logging */
#endif
};

/*------------------------------------------------------------------------*/

struct Parser
{
  Limmat *limmat;
  int num_specified_clauses, num_specified_max_id;
  Stack literals;
  FILE *file;
  int close_file;		/* do not close if 'file' was 'stdin' */
  int lineno;			/* lineno */
  char *error;			/* parse error occured */
  char *name;			/* remember file name for error message */
};

/*------------------------------------------------------------------------*/
/* We want to keep 'align' as macro to be able to have the result of 'align'
 * as compile time constant.  One reason for this is to speed up the
 * variable iterator.
 */
#define is_aligned(_n_,_alignment_) !((_n_) & ((_alignment_) - 1))

#define align(_n_,_alignment_) \
  (is_aligned(_n_,_alignment_) ? (_n_) : ((_n_)|((_alignment_) - 1)) + 1)

/*------------------------------------------------------------------------*/
/* Increment variable pointer during variable arena traversal.
 */
#define next_Variable(_v_) \
  ((Variable*)(align(sizeof(Variable), VARIABLE_ALIGNMENT) + (char*)(_v_)))

/*------------------------------------------------------------------------*/
/* Traverse all variables with non zero id.   The 2nd and 3rd argument
 * should be of type 'Variable*'.  The helper variable '_end_of_variables_'
 * is used for speeding up traversal.
 *
 * ATTENTION: These loops can not be nested.
 */
#define forall_variables(_limmat_,_pointer_to_variable_,_end_of_variables_) \
  for(_pointer_to_variable_ = start_of_Arena(&_limmat_->variables), \
	_end_of_variables_ = (Variable*) end_of_Arena(&_limmat_->variables); \
      _pointer_to_variable_ < (Variable*) _end_of_variables_; \
      _pointer_to_variable_ = next_Variable(_pointer_to_variable_)) \
    if(_pointer_to_variable_->id)

/*------------------------------------------------------------------------*/
/* Traverse the contents of a stack.  Since 'p' actually points to the
 * stack entries in the stack, you can use this loop for changing the
 * contents of the stack as well.
 *
 *  '_type_'             the element type pushed on the stack
 *  '_ptr_in_stack_'     a pointer to elements on the stack
 *  '_end_of_stack_'     a helper variable to speed up the iterator
 *
 * Typical usage would be
 *
 *   int * p, * end;
 *   Stack stack;
 *   init_Stack(limmat,&stack);
 *   push_Stack(limmat,&stack,(void*)0);   // actually push an int
 *   push_Stack(limmat,&stack,(void*)1);   // actually push an int
 *   push_Stack(limmat,&stack,(void*)2);   // actually push an int
 *   ...
 *   s = 0;
 *   forall_Stack(&stack, int, p, end);
 *     s += (int) *p;
 *
 * ATTENTION: These loops can not be nested.  In case you need to nest two
 * loops simply use a static inlined function for the inner loop.
 */
#define forall_Stack(_stack_,_type_,_ptr_in_stack_,_end_of_stack_) \
  for(_ptr_in_stack_ = (_type_*) start_of_Stack(_stack_), \
        _end_of_stack_ = (_type_*) end_of_Stack(_stack_); \
      _ptr_in_stack_ < _end_of_stack_; \
      _ptr_in_stack_++)

/*-----------------------------------------------------------------------*/
/* Traverse all clauses.
 *
 * ATTENTION: These loops can not be nested.
 */
#define forall_clauses(_limmat_,_ptr_to_clause_,_end_of_clauses_) \
  forall_Stack(&(_limmat_)->clauses,Clause*,_ptr_to_clause_,_end_of_clauses_)

/*------------------------------------------------------------------------*/
/* Traverse the literals of a clause.
 *
 * ATTENTION: These loops can not be nested.
 */
#define forall_literals(_limmat_,_clause_,_ptol_,_eol_) \
  for(_ptol_ = clause2literals(_limmat_, _clause_), \
       _eol_ = (_clause_)->size + _ptol_; \
      _ptol_ < _eol_; \
      _ptol_++)

/*------------------------------------------------------------------------*/

static int
min (int a, int b)
{
  return (a < b) ? a : b;
}

/*------------------------------------------------------------------------*/
#ifdef LIMMAT_WHITE
/*------------------------------------------------------------------------*/

inline static int
msnzb (int n)
{
  int res, i, tmp, mask;

  res = 0;
  i = sizeof (int) * 4;
  mask = (~0) << i;

  while (i)
    {
      tmp = n & mask;
      if (tmp)
	{
	  n = tmp >> i;
	  res += i;
	}
      i /= 2;
      mask >>= i;
    }

  return res;
}

/*------------------------------------------------------------------------*/

inline static int
log2ceil (int n)
{
  int res;

  assert (n > 0);

  res = msnzb (n);
  if (n > (1 << res))
    res++;

  assert ((1 << res) >= n);
  assert ((1 << res) / 2 < n);

  return res;
}

/*------------------------------------------------------------------------*/
#endif /* LIMMAT_WHITE */
/*------------------------------------------------------------------------*/
/* Get the number of seconds spent in this process including system time.
 */
static double
get_time (void)
{
  struct rusage u;
  double res;

  if (getrusage (RUSAGE_SELF, &u))
    res = 0;
  else
    {
      res = 0;
      res += u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
      res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
    }

  return res;
}

/*------------------------------------------------------------------------*/
/* To measure the time spent by a particular procedure, save the current
 * process time with 'start_timer'.  After the procedure has finished call
 * 'stop_timer' with the previously saved time.  The result is the time
 * between both calls.
 */
static void
start_timer (double *timer)
{
  *timer = get_time ();
}

/*------------------------------------------------------------------------*/

static double
stop_timer (double timer)
{
  double res;

  res = get_time () - timer;
  if (res < 0)
    res = 0;

  return res;
}

/*------------------------------------------------------------------------*/
/* Combine both operations 'start_timer' and 'stop_timer'.  The accumulated
 * time is not added to the result for the previous time interval.
 */
static double
restart_timer (double *timer)
{
  double res;

  res = stop_timer (*timer);
  start_timer (timer);

  return res;
}

/*------------------------------------------------------------------------*/

void
adjust_timer_Limmat (Limmat * limmat)
{
  limmat->time += restart_timer (&limmat->timer);
}

/*------------------------------------------------------------------------*/

static void
init_Counter (Counter * counter, int init, double factor, int increment)
{
  assert (init >= 0);

  counter->factor = factor;
  counter->init = init;
  counter->inc = increment;
  counter->current = init;
  counter->finished = 0;
}

/*------------------------------------------------------------------------*/

static void
dec_Counter (Counter * counter)
{
  assert (counter->current >= 0);
  if (counter->init && counter->current >= 0)
    counter->current -= 1;
}

/*------------------------------------------------------------------------*/

static int
done_Counter (Counter * counter)
{
  int res;

  if (counter->init)
    {
      if (counter->current)
	res = 0;
      else
	{
	  counter->init *= counter->factor;
	  counter->init += counter->inc;
	  counter->current = counter->init;
	  counter->finished++;
	  res = 1;
	}
    }
  else
    res = 0;

  return res;
}

/*------------------------------------------------------------------------*/
#ifdef CHECK_CONSISTENCY
/*------------------------------------------------------------------------*/
/* This section contains some helper functions in case we want to check that
 * allocated data is released with the correct size as argument to delete.
 * Without this check we would not know whether our internal count for the
 * used up memory is correct.
 */
/*------------------------------------------------------------------------*/

static int
bucketsize (size_t n)
{
  return n + DATA_OFFSET;
}

/*------------------------------------------------------------------------*/

static Bucket *
ptr2bucket (void *ptr)
{
  Bucket *res;

  res = (Bucket *) (((char *) ptr) - DATA_OFFSET);
  assert (ptr == (void *) &res->aligned_data);

  return res;
}

/*------------------------------------------------------------------------*/

static void *
bucket2ptr (Bucket * bucket)
{
  return (void *) &bucket->aligned_data;
}

/*------------------------------------------------------------------------*/
#endif /* CHECK_CONSISTENCY */
/*------------------------------------------------------------------------*/

static void *
new (Limmat * limmat, size_t size)
{
  size_t bytes;
  void *res;

#ifdef CHECK_CONSISTENCY
  Bucket *bucket;

  bytes = bucketsize (size);
  bucket = (Bucket *) malloc (bytes);
  assert (bucket);
  bucket->size = size;
  res = bucket2ptr (bucket);
#else
  bytes = size;
  res = malloc (size);
#endif

  limmat->bytes += bytes;
  if (limmat->max_bytes < limmat->bytes)
    limmat->max_bytes = limmat->bytes;

  return res;
}

/*------------------------------------------------------------------------*/

static void
delete (Limmat * limmat, void *ptr, size_t size)
{
  size_t bytes;

#ifdef CHECK_CONSISTENCY
  Bucket *bucket;

  bucket = ptr2bucket (ptr);
  assert (bucket->size == size);
  bytes = bucketsize (bucket->size);
  free (bucket);
#else
  free (ptr);
  bytes = size;
#endif

  assert (limmat->bytes >= bytes);
  limmat->bytes -= bytes;
}

/*------------------------------------------------------------------------*/

static void *
resize (Limmat * limmat, void *ptr, size_t new_size, size_t old_size)
{
  size_t old_bytes, new_bytes;
  void *res;

#ifdef CHECK_CONSISTENCY
  Bucket *old_bucket, *new_bucket;

  old_bucket = ptr2bucket (ptr);
  assert (old_bucket->size == old_size);
  old_bytes = bucketsize (old_bucket->size);
  new_bytes = bucketsize (new_size);

  new_bucket = (Bucket *) realloc (old_bucket, new_bytes);
  assert (new_bucket);
  new_bucket->size = new_size;
  res = bucket2ptr (new_bucket);
#else
  res = realloc (ptr, new_size);
  assert (res);
  old_bytes = old_size;
  new_bytes = new_size;
#endif

  assert (limmat->bytes >= old_bytes);

  limmat->bytes -= old_bytes;
  limmat->bytes += new_bytes;

  if (old_bytes > new_bytes && limmat->max_bytes < limmat->bytes)
    limmat->max_bytes = limmat->bytes;

  return res;
}

/*------------------------------------------------------------------------*/

static int
count_Stack (Stack * stack)
{
  return stack->internal_count;
}

/*------------------------------------------------------------------------*/

static void
reset_Stack (Limmat * limmat, Stack * stack, int new_count)
{
  assert (new_count >= 0);
  assert (new_count <= count_Stack (stack));

  stack->internal_count = new_count;
}

/*------------------------------------------------------------------------*/

static void
init_Stack (Limmat * limmat, Stack * stack)
{
#ifdef COMPACT_STACK
  stack->log_size = 0;
#else
  stack->internal_size = 1;
#endif

  stack->internal_count = 0;
  stack->data.as_pointer = 0;
}

/*------------------------------------------------------------------------*/

static int
size_Stack (Stack * stack)
{
  int res;

#ifdef COMPACT_STACK
  res = 1 << stack->log_size;
#else
  res = stack->internal_size;
#endif

  return res;
}

/*------------------------------------------------------------------------*/

static void
release_Stack (Limmat * limmat, Stack * stack)
{
  size_t bytes;

  if (size_Stack (stack) > 1)
    {
      bytes = size_Stack (stack) * sizeof (void *);
      delete (limmat, stack->data.as_array, bytes);
    }
}

/*------------------------------------------------------------------------*/

static void
enlarge_Stack (Limmat * limmat, Stack * stack)
{
  size_t old_bytes, new_bytes;
  int old_size, new_size;
  void *data;

  old_size = size_Stack (stack);
  new_size = old_size * 2;
  new_bytes = new_size * sizeof (void *);

  if (old_size == 1)
    {
      data = stack->data.as_pointer;
      stack->data.as_array = (void **) new (limmat, new_bytes);
      stack->data.as_array[0] = data;
    }
  else
    {
      assert (old_size > 1);
      old_bytes = old_size * sizeof (void *);
      stack->data.as_array = (void **)
	resize (limmat, stack->data.as_array, new_bytes, old_bytes);
    }

#ifdef COMPACT_STACK
  if (stack->log_size + 1 >= LOG_MAX_STACK_SIZE)
    abort ();
  if (stack->internal_count + 1 >= MAX_STACK_COUNT)
    abort ();
  stack->log_size++;
#else
  stack->internal_size = new_size;
#endif
}

/*------------------------------------------------------------------------*/

inline static int
is_immediate_Stack (Stack * stack)
{
#ifdef COMPACT_STACK
  return stack->log_size == 0;
#else
  return stack->internal_size <= 1;
#endif
}

/*------------------------------------------------------------------------*/

inline static void *
pop (Stack * stack)
{
  void *res;

  assert (stack->internal_count);

  stack->internal_count -= 1;

  if (is_immediate_Stack (stack))
    res = stack->data.as_pointer;
  else
    res = stack->data.as_array[stack->internal_count];

  return res;
}

/*------------------------------------------------------------------------*/

inline static int
is_full_Stack (Stack * stack)
{
  return stack->internal_count >= size_Stack (stack);
}

/*------------------------------------------------------------------------*/

inline static void
push (Limmat * limmat, Stack * stack, void *data)
{
  if (is_full_Stack (stack))
    enlarge_Stack (limmat, stack);

  assert (stack->internal_count < size_Stack (stack));

  if (is_immediate_Stack (stack))
    stack->data.as_pointer = data;
  else
    stack->data.as_array[stack->internal_count] = data;

  stack->internal_count += 1;
}

/*------------------------------------------------------------------------*/

inline static void *
start_of_Stack (Stack * stack)
{
  void *res;

  switch (size_Stack (stack))
    {
    case 0:
      res = 0;
      break;

    case 1:
      res = &stack->data.as_pointer;
      break;

    default:
      res = stack->data.as_array;
      break;
    }

  return res;
}

/*------------------------------------------------------------------------*/

inline static void *
end_of_Stack (Stack * stack)
{
  void *res, **base;

  base = start_of_Stack (stack);
  res = base + stack->internal_count;

  return res;
}

/*------------------------------------------------------------------------*/

inline static int *
intify_Stack (Stack * stack)
{
  void ** start;

  start = start_of_Stack (stack);

#ifdef WIDE_POINTERS
    {
      void ** p, ** end;
      int * q, tmp;

      end = end_of_Stack (stack);

      for(p = start, q = (int*) start ; p < end; p++, q++)
        {
	  tmp = (int) (PTR_SIZED_WORD) *p;
	  *q = tmp;
	}
    }
#endif

  return (int*) start;
}

/*------------------------------------------------------------------------*/
#ifndef NDEBUG
/*------------------------------------------------------------------------*/

static int
is_valid_alignment (PTR_SIZED_WORD a)
{
  return a == 4 || a == 8 || a == 16 || a == 32 || a == 64;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
#if !defined(NDEBUG) || defined(LIMMAT_WHITE)
/*------------------------------------------------------------------------*/

static int
is_aligned_ptr (void *ptr, PTR_SIZED_WORD alignment)
{
  return is_aligned ((PTR_SIZED_WORD) ptr, alignment);
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

static void *
align_ptr (void *ptr, PTR_SIZED_WORD alignment)
{
  void *res;

  res = (void *) align ((PTR_SIZED_WORD) ptr, alignment);
  assert (is_aligned_ptr (res, alignment));

  return res;
}

/*------------------------------------------------------------------------*/
#if defined(CHECK_CONSISTENCY) || defined(EXPENSIVE_CHECKS)
/*------------------------------------------------------------------------*/

static int
check_it_now (Limmat * limmat)
{
  int res;

  assert (limmat->check_counter >= 0);
  if (limmat->check_counter > 0)
    limmat->check_counter--;
  res = !limmat->check_counter;
#ifdef EXPENSIVE_CHECKS
  if (res)
    res = !limmat->inner_mode;
#endif

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

static void
invariant_Arena (Limmat * limmat, Arena * arena)
{
#ifndef NDEBUG

  assert (arena->mem_start <= arena->data_start);
  assert (arena->data_start <= arena->data_end);
  assert (arena->data_end <= arena->mem_end);

  assert (is_aligned_ptr (arena->data_start, arena->alignment));
  assert (is_aligned_ptr (arena->data_end, arena->alignment));

#ifdef EXPENSIVE_CHECKS
  if (check_it_now (limmat))
    {
      char *p;
      for (p = arena->data_end; p < arena->mem_end; p++)
	assert (!*p);
    }
#endif
#endif
}

/*------------------------------------------------------------------------*/

static void
init_Arena (Limmat * limmat, Arena * arena, int alignment)
{
  assert (is_valid_alignment ((PTR_SIZED_WORD) alignment));

  arena->alignment = alignment;
  arena->mem_start = 0;
  arena->mem_end = 0;
  arena->data_start = 0;
  arena->data_end = 0;
}

/*------------------------------------------------------------------------*/

static void
release_Arena (Limmat * limmat, Arena * arena)
{
  size_t bytes;

  if (arena->mem_start)
    {
      bytes = arena->mem_end - arena->mem_start;
      assert (bytes > 0);
      delete (limmat, arena->mem_start, bytes);
    }
}

/*------------------------------------------------------------------------*/

static void
reset_Arena (Limmat * limmat, Arena * arena)
{
  char *p;
  for (p = arena->data_start; p < arena->data_end; p++)
    *p = 0;
  arena->data_end = arena->data_start;
}

/*------------------------------------------------------------------------*/

static void
limmat_memcpy (char *dst, const char *src, size_t size)
{
  const char *p, *end;
  char *q;

  assert (size >= 0);

  if (src > dst)
    {
      p = src;
      q = dst;
      end = src + size;
      while (p < end)
	*q++ = *p++;
    }
  else if (src < dst)
    {
      p = src + size;
      q = dst + size;
      while (p > src)
	*--q = *--p;
    }
}

/*------------------------------------------------------------------------*/

static void *
alloc_Arena (Limmat * limmat, Arena * arena, int size, size_t *delta_ptr)
{
  char *res, *new_data_start, *new_data_end, *new_mem_start, *p, *q;
  size_t mem_delta, data_delta, old_bytes, new_bytes, required_bytes;
  size_t old_alignment, new_alignment, delta_alignment;

  size = align (size, arena->alignment);
  data_delta = 0;

  /* First adjust allocated memory to hold current data and at least
   * additional 'size' bytes.
   */
  if (arena->mem_start)
    {
      /* There has been a previous allocation.
       */
      new_data_end = arena->data_end + size;

      if (new_data_end > arena->mem_end)
	{
	  /* The allocated memory can not hold additional 'size' bytes.
	   */
#ifdef EXPENSIVE_CHECKS
	  int len = arena->data_end - arena->data_start;
	  char *old_data = memmove (malloc (len), arena->data_start, len);
#endif

	  old_bytes = arena->mem_end - arena->mem_start;
	  new_bytes = 2 * old_bytes;
	  required_bytes = old_bytes + size + arena->alignment;
	  while (new_bytes < required_bytes)
	    new_bytes *= 2;

	  new_mem_start = (char *)
	    resize (limmat, arena->mem_start, new_bytes, old_bytes);
	  mem_delta = new_mem_start - arena->mem_start;

	  if (mem_delta)
	    {
	      new_data_start = align_ptr (new_mem_start, arena->alignment);
	      data_delta = new_data_start - arena->data_start;
	      old_alignment = arena->data_start - arena->mem_start;
	      new_alignment = new_data_start - new_mem_start;
	      delta_alignment = new_alignment - old_alignment;

	      if (delta_alignment)
		{
		  new_data_end = arena->data_end + data_delta;

		  /* Be carefull in case there is a clever 'realloc' and a
		   * reallocated block overlaps with the original block, and
		   * one block is aligned and the other not.  In this case
		   * it is important to have a proper implementation of
		   * 'memcpy' that can handle overlapping source and
		   * destination data blocks.
		   */
		  limmat_memcpy (new_data_start,
				 new_data_start - delta_alignment,
				 new_data_end - new_data_start);
		}
	    }
	  else
	    {
	      new_data_start = arena->data_start;	/* see assertion below */
	      data_delta = 0;
	    }

	  arena->mem_start = new_mem_start;
	  arena->mem_end = new_mem_start + new_bytes;

	  arena->data_start += data_delta;
	  arena->data_end += data_delta;

	  assert (arena->data_start == new_data_start);

	  /* Clear all fresh allocated data.
	   */
	  p = arena->data_end;
	  q = arena->mem_end;
	  while (p < q)
	    *p++ = 0;

#ifdef EXPENSIVE_CHECKS
	  assert (!memcmp (old_data, arena->data_start, len));
	  free (old_data);
#endif
	}
    }
  else
    {
      /* No previous allocation.
       */
      required_bytes = size + arena->alignment;
      new_bytes = 16;
      while (new_bytes < required_bytes)
	new_bytes *= 2;

      new_mem_start = (char *) new (limmat, new_bytes);
      new_data_start = align_ptr (new_mem_start, arena->alignment);
      arena->mem_start = new_mem_start;
      arena->mem_end = new_mem_start + new_bytes;
      arena->data_start = new_data_start;
      arena->data_end = new_data_start;

      /* Clear all allocated data.
       */
      p = arena->mem_start;
      q = arena->mem_end;
      while (p < q)
	*p++ = 0;
    }

  res = arena->data_end;
  arena->data_end += size;

  invariant_Arena (limmat, arena);
  assert (is_aligned_ptr (res, arena->alignment));

  *delta_ptr = data_delta;

  return res;
}

/*------------------------------------------------------------------------*/

static void *
start_of_Arena (Arena * arena)
{
  return arena->data_start;
}

/*------------------------------------------------------------------------*/

static void *
end_of_Arena (Arena * arena)
{
  return arena->data_end;
}

/*------------------------------------------------------------------------*/
#ifndef NDEBUG
/*------------------------------------------------------------------------*/

static int
is_array_Arena (Arena * arena, int element_size)
{
  int has_non_zero_rest;
  size_t data_bytes;

  element_size = align (element_size, arena->alignment);
  data_bytes = arena->data_end - arena->data_start;
  has_non_zero_rest = data_bytes % element_size;

  return !has_non_zero_rest;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
#if 0
/*------------------------------------------------------------------------*/

static void *
next_Arena (Arena * arena, void *ptr, int element_size)
{
  void *res;

  assert (is_array_Arena (arena, element_size));
  assert (is_aligned_ptr (ptr, arena->alignment));
  assert (arena->mem_start <= (char *) ptr);
  assert (arena->mem_end > (char *) ptr);

  res = align (element_size, arena->alignment) + (char *) ptr;

  assert (is_aligned_ptr (res, arena->alignment));

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* An Arena can be accessed as an array.
 */
static void *
access_Arena (Limmat * limmat,
	      Arena * arena, int index, int element_size, size_t *delta_ptr)
{
  int old_num_elements, new_num_elements, delta_num_elements;
  size_t delta_bytes;
  char *res;

  assert (index >= 0);
  assert (element_size > 0);
  assert (is_array_Arena (arena, element_size));

  element_size = align (element_size, arena->alignment);

  old_num_elements = (arena->data_end - arena->data_start) / element_size;
  new_num_elements = index + 1;
  delta_num_elements = new_num_elements - old_num_elements;

  if (delta_num_elements > 0)
    {
      delta_bytes = delta_num_elements * element_size;
      (void) alloc_Arena (limmat, arena, delta_bytes, delta_ptr);
      assert (is_array_Arena (arena, element_size));
    }
  else
    *delta_ptr = 0;

  res = arena->data_start + element_size * index;
  assert (res + element_size <= arena->data_end);

  return res;
}

/*------------------------------------------------------------------------*/

static int
getbit (void *v, PTR_SIZED_WORD bit)
{
  return (bit & (PTR_SIZED_WORD) v) != 0;
}

/*------------------------------------------------------------------------*/

static void *
clrbit (void *v, PTR_SIZED_WORD bit)
{
  PTR_SIZED_WORD tmp;
  void *res;

  tmp = (PTR_SIZED_WORD) v;
  tmp &= ~bit;
  res = (void *) tmp;

  return res;
}

/*------------------------------------------------------------------------*/

static void *
toggle (void *v, PTR_SIZED_WORD bit)
{
  PTR_SIZED_WORD tmp;
  void *res;

  tmp = (PTR_SIZED_WORD) v;
  tmp ^= bit;
  res = (void *) tmp;

  return res;
}

/*------------------------------------------------------------------------*/

static int
is_signed (void *v)
{
  return getbit (v, SIGN_BIT);
}
static void *
strip (void *v)
{
  return clrbit (v, SIGN_BIT);
}
static void *
not (void *v)
{
  return toggle (v, SIGN_BIT);
}

/*------------------------------------------------------------------------*/

static void *
lit2var (void *v)
{
  return clrbit (v, ALL_BUT_SIGN_BITS);
}

/*------------------------------------------------------------------------*/

static int
is_assigned (Variable * v)
{
  if (is_signed (v))
    v = not (v);
  return v->assignment != v;
}

/*------------------------------------------------------------------------*/

static Variable *
deref (Variable * v)
{
  Variable *res;
  int sign;

  if ((sign = is_signed (v)))
    v = not (v);
  res = v->assignment;
  if (sign)
    res = not (res);

  return res;
}

/*------------------------------------------------------------------------*/
/* The assignments are stored in a cyclic queue.  The 'head_assignments'
 * index points to the next assignment to be dequeued, while the
 * 'tail_assignments' index points to the next empty slot.  Both keep on
 * increasing starting from zero and wrap around at 'size_assignments'.
 */
static int
count_assignments (Limmat * limmat)
{
  int res;

  if (limmat->tail_assignments >= limmat->head_assignments)
    {
      res = limmat->tail_assignments - limmat->head_assignments;
    }
  else
    {
      res = limmat->size_assignments - limmat->head_assignments;
      res += limmat->tail_assignments;
    }

  assert (0 <= res);
  assert (res < limmat->size_assignments);

  return res;
}

/*------------------------------------------------------------------------*/
/* Increase the size of the buffer allocated for the assignments queue by a
 * factor of two.
 */
static void
resize_assignments (Limmat * limmat)
{
  int new_size, new_tail, new_bytes, old_size, old_bytes, i, old_pos;
  Assignment *new_assignments, *old_assignments;
  Variable *literal;

  old_size = limmat->size_assignments;
  new_size = old_size * 2;
  new_bytes = sizeof (Assignment) * new_size;
  new_assignments = (Assignment *) new (limmat, new_bytes);
  new_tail = count_assignments (limmat);

  assert (new_tail < new_size);

  old_assignments = limmat->assignments;

  /* We move the whole contents of the queue to the start of the new
   * allocated buffer. Then we have to adjust the 'position' index of all
   * variables of scheduled assignments.
   */
  for (i = 0; i < new_tail; i++)
    {
      old_pos = i + limmat->head_assignments;
      if (old_pos >= old_size)
	old_pos -= old_size;
      literal = strip (old_assignments[old_pos].literal);
#if defined(EARLY_CONFLICT_DETECTION) && defined(SHORTEN_REASONS)
      assert (literal->pos_on_queue == old_pos);
#endif
      new_assignments[i] = old_assignments[old_pos];
      literal->pos_on_queue = i;
    }

  old_bytes = sizeof (Assignment) * old_size;
  delete (limmat, old_assignments, old_bytes);

  limmat->assignments = new_assignments;
  limmat->size_assignments = new_size;
  limmat->tail_assignments = new_tail;
  limmat->head_assignments = 0;
}

/*------------------------------------------------------------------------*/

inline static void
fix_pointer (void *void_ptr_ptr, size_t delta)
{
  char **char_ptr_ptr, *new_value;

  char_ptr_ptr = (char **) void_ptr_ptr;
  new_value = delta + *char_ptr_ptr;
  *char_ptr_ptr = new_value;
}

/*------------------------------------------------------------------------*/

static void
fix_variables (Limmat * limmat, size_t delta)
{
  Variable *v, *eov;

  forall_variables (limmat, v, eov)
    if (v->assignment != TRUE && v->assignment != FALSE)
    {
      fix_pointer (&v->assignment, delta);
      assert (v->assignment == v);
    }
}

/*------------------------------------------------------------------------*/

static void
fix_Stack (Stack * stack, size_t delta)
{
  void **o, **eoo;

  forall_Stack (stack, void *, o, eoo) fix_pointer (o, delta);
}

/*------------------------------------------------------------------------*/

static Variable **
clause2literals (Limmat * limmat, Clause * clause)
{
  Variable **res;

  res = start_of_Stack (&limmat->literals);
  res += clause->literals_idx;

  return res;
}

/*------------------------------------------------------------------------*/

static void
fix_clauses (Limmat * limmat, size_t delta)
{
  Variable **q, **eol;
  Clause **p, **eoc;

  forall_clauses (limmat, p, eoc)
    forall_literals (limmat, *p, q, eol) fix_pointer (q, delta);
}

/*------------------------------------------------------------------------*/

static void
fix_pointers_to_variables (Limmat * limmat, size_t delta)
{
  assert (limmat->decision_level < 0);
  assert (!count_assignments (limmat));

  fix_variables (limmat, delta);
  fix_Stack (&limmat->order, delta);
  fix_clauses (limmat, delta);
}

/*------------------------------------------------------------------------*/
#ifndef NDEBUG
/*------------------------------------------------------------------------*/

static void
check_valid_assignment_index (Limmat * limmat, int idx)
{
  assert (0 <= idx);
  assert (idx <= limmat->size_assignments);

  if (limmat->head_assignments <= limmat->tail_assignments)
    {
      assert (idx >= limmat->head_assignments);
      assert (idx < limmat->tail_assignments);
    }
  else
    {
      assert (idx >= limmat->head_assignments ||
	      idx < limmat->tail_assignments);
    }
}

/*------------------------------------------------------------------------*/

static void
check_assignment_index (Limmat * limmat, Variable * literal)
{
  int sign;

  sign = is_signed (literal);
  if (sign)
    literal = not (literal);
  check_valid_assignment_index (limmat, literal->pos_on_queue);
  if (sign)
    assert (literal->sign_on_queue);
  else
    assert (!literal->sign_on_queue);
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
#ifdef SHORTEN_REASONS
/*------------------------------------------------------------------------*/
/* Compare assignments with respect to level and size of reasons.
 */
static int
cmp_assignment (Limmat * limmat, Assignment * a, Assignment * b)
{
  int res, alevel, blevel, aid, bid;
  Occurence areason, breason;

  alevel = a->decision_level;
  blevel = b->decision_level;

  if (alevel < blevel)
    res = -1;
  else if (alevel > blevel)
    res = 1;
  else
    {
      areason = a->reason;
      breason = b->reason;

      if (!areason)
	{
	  if (breason)
	    res = -1;
	  else
	    res = 0;
	}
      else if (!breason)
	res = 1;
      else
	{
/* Comment this out out to make comparison with indexed version easier.
 */
#if 1
	  int asize, bsize;

	  asize = areason->size;
	  bsize = breason->size;

	  if (asize < bsize)
	    res = -1;
	  else if (asize > bsize)
	    res = 1;
	  else
#endif
	    {
	      aid = areason->id;
	      bid = breason->id;
	      res = 0;
	      if (aid < bid)
		res = -1;
	      else if (aid > bid)
		res = 1;
	    }
	}
    }

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Push a new assignment onto the assignment queue for later retrieval with
 * 'pop_assignment'.  The queue is a FIFO queue for implementing BFS.  This
 * should heuristically generate smaller conflict generating assignment
 * sequences than DFS with potentially smaller conflict clauses.
 */
static void
push_assignment (Limmat * limmat, Assignment * assignment)
{
  Variable *literal, *stripped, *tmp;
  int sign, tail, count;

#ifdef SHORTEN_REASONS
  Assignment *old_assignment;
  int cmp;
#endif

#ifdef LOG_PUSH
  static int counter = 0;
  static int var2int (Variable *);
  fprintf (LOGFILE (limmat), "PUSH\t%d\t%d\t%d\n",
	   counter++, var2int (assignment->literal),
	   count_assignments (limmat));
#endif

  if (limmat->inconsistent >= 0)
    {
      /* Just skip additional assignments after the assignment queue
       * detected that two contradicting assignments have already been
       * pushed in an earlier 'push_assignment' operation.
       */
      return;
    }

  literal = assignment->literal;
  sign = is_signed (literal);
  stripped = sign ? not (literal) : literal;

  tmp = deref (literal);
  assert (tmp != FALSE);

#ifndef NDEBUG
  if (stripped->pos_on_queue >= 0)
    {
      if (!stripped->sign_on_queue)
	check_assignment_index (limmat, stripped);
      else
	check_assignment_index (limmat, not (stripped));
    }
#endif

  if (tmp != TRUE)
    {
#ifdef SHORTEN_REASONS
      if (stripped->pos_on_queue >= 0 &&	/* var already scheduled */
	  !sign == !stripped->sign_on_queue)	/* with matching sign */
	{
	  /* In case the same assignment is already scheduled and it has
	   * a larger decision level than the current one, or it has the
	   * same decision level but the reason is smaller, then we are
	   * overwriting the old assignment with the current one.  Otherwise
	   * we keep the old assignment and just skip this one.
	   */
	  old_assignment = limmat->assignments + stripped->pos_on_queue;
	  assert (old_assignment->literal == assignment->literal);

	  cmp = cmp_assignment (limmat, assignment, old_assignment);
	  if (!cmp)
	    assert (limmat->decision_level == -1);

	  if (cmp < 0)
	    {
	      old_assignment->decision_level = assignment->decision_level;
	      old_assignment->reason = assignment->reason;
	    }
	}
      else
#endif
	{
	  /* The variable of the assignment is not scheduled to be assigned
	   * or it was scheduled to be assigned with opposite value.  In any
	   * case, we push a fresh assignment on the assignment queue making
	   * sure that there is enough space for scheduling the current
	   * assignment.  In particular the queue can maximally hold
	   * 'limmat->size_assignments - 1' elements.
	   */
	  count = count_assignments (limmat);
	  if (count + 1 >= limmat->size_assignments)
	    {
	      resize_assignments (limmat);
	      assert (count_assignments (limmat) == count);
	    }

	  tail = limmat->tail_assignments;

#ifdef EARLY_CONFLICT_DETECTION
	  if (stripped->pos_on_queue >= 0 &&	/* variable scheduled */
	      !sign != !stripped->sign_on_queue)	/* with opposite sign */
	    {
	      /* The variable has been scheduled to be assigned to the
	       * opposite value already.  We just set the 'inconsistent'
	       * flag to point to the position of this earlier assignment,
	       * such that 'propagate' will immediately use the old and this
	       * new assignment in sequence to generate a conflict as early
	       * as possible.
	       */
	      assert (stripped->pos_on_queue >= 0);
	      limmat->inconsistent = stripped->pos_on_queue;
	    }
	  else
#endif
	    {
	      stripped->pos_on_queue = tail;
	      stripped->sign_on_queue = sign ? 1 : 0;
	    }

	  /* Enqueue the current assignment at the tail of the assignment
	   * queue.  The 'tail' is increased and may wrap back to '0' at the
	   * end of the buffer allocated for the queue.
	   */
	  limmat->assignments[tail++] = *assignment;
	  if (tail >= limmat->size_assignments)
	    tail = 0;
	  limmat->tail_assignments = tail;

	  assert (count_assignments (limmat) == count + 1);
	}
    }
}

/*------------------------------------------------------------------------*/
/* Pop a previously scheduled assignment from the the assignments queue.
 */
static void
pop_assignment (Limmat * limmat, Assignment * assignment)
{
  int pos, head, tail, size, inconsistent;
  Assignment *assignments;
  Variable *stripped;

#ifndef NDEBUG
  int count = count_assignments (limmat);
  assert (count > 0);
#endif

  assignments = limmat->assignments;
  head = limmat->head_assignments;
  size = limmat->size_assignments;
  inconsistent = limmat->inconsistent;

  if (inconsistent >= 0)
    {
      /* A previous 'push_assignment' detected two conflicting
       * 'assignments'.  The first can be found at position 'inconsistent'
       * and the second is the last assignment at the tail of the queue.  We
       * first schedule the propagation of the first assignment at position
       * 'inconsistent'.
       */
      assert (inconsistent < size);
      pos = inconsistent;
      tail = limmat->tail_assignments - 1;
      if (tail < 0)
	tail = size - 1;

      assert (tail < limmat->size_assignments);
      assert (limmat->inconsistent != tail);
      assert (assignments[pos].literal == not (assignments[tail].literal));

      /* Remember that we already propagated the first assignment and still
       * need to propagate the last one.  The queue itself is not changed.
       * In particular its size is not decreased.
       */
      limmat->inconsistent = tail;
    }
  else
    {
      pos = head++;
      if (head >= size)
	head = 0;
      limmat->head_assignments = head;

      assert (count_assignments (limmat) + 1 == count);
    }

  /* Do not forget to invalidate the 'position' for the variable.
   */
  assert (0 <= pos);
  assert (pos < limmat->size_assignments);
  stripped = strip (assignments[pos].literal);
  stripped->sign_on_queue = 0;
  stripped->pos_on_queue = -1;

  *assignment = assignments[pos];
}

/*------------------------------------------------------------------------*/
/* During backtrakcing, after a conflict has been generated, we have to
 * reset the assignments queue.  As with stack we never decrease the size of
 * the allocated buffer itself.
 */
static void
reset_assignments (Limmat * limmat)
{
  int sign, pos, i, count, size, head;
  Assignment *assignments;
  Variable *literal;

  assignments = limmat->assignments;
  count = count_assignments (limmat);
  size = limmat->size_assignments;
  head = limmat->head_assignments;

  for (i = 0; i < count; i++)
    {
      pos = i + head;
      if (pos >= size)
	pos -= size;
      literal = assignments[pos].literal;
      sign = is_signed (literal);
      if (sign)
	literal = not (literal);

#ifndef NDEBUG
      {
	if (literal->pos_on_queue >= 0)
	  {
#if defined(EARLY_CONFLICT_DETECTION) && defined(SHORTEN_REASONS)
	    assert (literal->pos_on_queue == pos);
	    assert (!literal->sign_on_queue == !sign);
#endif
	  }
      }
#endif

      literal->sign_on_queue = 0;
      literal->pos_on_queue = -1;
    }

  limmat->tail_assignments = 0;
  limmat->head_assignments = 0;
  limmat->inconsistent = -1;

  assert (count_assignments (limmat) == 0);
}

/*------------------------------------------------------------------------*/

static Statistics *
new_Statistics (Limmat * limmat)
{
  Statistics *res;

  res = (Statistics *) new (limmat, sizeof (Statistics));

  res->removed_clauses = 0;
  res->removed_literals = 0;

  res->max_literals = 0;
  res->learned_literals = 0;
  res->original_literals = 0;

  res->original_clauses = 0;
  res->learned_clauses = 0;
  res->max_clauses = 0;
  res->rescored_variables = 0;
  res->sum_assigned_in_decision = 0;
  res->sum_score_in_decision = 0;

  res->assignments = 0;
  res->propagations = 0;
  res->visits = 0;
  res->uips = 0;
  res->backjumps = 0;

  res->swapped = 0;
  res->compared = 0;
  res->searched = 0;

  return res;
}

/*------------------------------------------------------------------------*/

static int
option (const char *name, int default_initialization)
{
  char *val;
  int res;

  val = getenv (name);
  if (val)
    res = atoi (val);
  else
    res = default_initialization;

  return res;
}

/*------------------------------------------------------------------------*/

static double
foption (const char *name, double default_initialization)
{
  double res;
  char *val;

  val = getenv (name);
  if (val)
    res = atof (val);
  else
    res = default_initialization;

  return res;
}

/*------------------------------------------------------------------------*/

Limmat *
new_Limmat (FILE * log)
{
  Limmat *res;
  int bytes;

  res = (Limmat *) malloc (sizeof (Limmat));
  res->bytes = 0;
  res->max_bytes = 0;

  init_Stack (res, &res->clauses);
  init_Stack (res, &res->units);
  init_Stack (res, &res->literals);
  res->contains_empty_clause = 0;
  res->added_clauses = 0;
  res->num_clauses = 0;

  res->max_id = 0;
  res->added_literals = 0;
  res->num_literals = 0;
  res->num_variables = 0;
  init_Arena (res, &res->variables, VARIABLE_ALIGNMENT);

  res->decision_level = -1;
  res->control_size = 1;
  bytes = sizeof (Frame) * res->control_size;
  res->control = (Frame *) new (res, bytes);

  res->size_assignments = 1;
  bytes = sizeof (Assignment) * res->size_assignments;
  res->assignments = (Assignment *) new (res, bytes);
  res->head_assignments = 0;
  res->tail_assignments = 0;

  init_Stack (res, &res->stack);
  init_Stack (res, &res->trail);
  init_Stack (res, &res->clause);
  init_Stack (res, &res->order);
  res->last_assigned_pos = -1;
  res->first_zero_pos = 0;
  res->num_assigned = 0;
  res->max_score = 0;
  res->current_max_score = 0;

  res->inconsistent = -1;

  res->error = 0;
  res->time = 0;

  res->log = log;

  init_Counter (&res->report, 99, 1.21, 0);
  init_Counter (&res->restart, option ("RESTART", RESTART), 1.4142, 1);
  init_Counter (&res->rescore, option ("RESCORE", RESCORE), 1, 0);

  res->score_factor = foption ("RESCOREFACTOR", RESCOREFACTOR);

  res->num_decisions = 0;
  res->num_conflicts = 0;

  res->stats = 0;
  if (option ("STATISTICS", STATISTICS))
    res->stats = new_Statistics (res);

#if defined(CHECK_CONSISTENCY) || defined(EXPENSIVE_CHECKS)
  res->check_counter = option ("CHECK", 0);
#endif

#ifdef LOG_SOMETHING
  res->dont_log = 0;
#endif

#ifdef EXPENSIVE_CHECKS
  res->inner_mode = 0;
#endif

  return res;
}

/*------------------------------------------------------------------------*/

void
set_log_Limmat (Limmat * limmat, FILE * log)
{
  limmat->log = log;
}

/*------------------------------------------------------------------------*/

static int
get_score (Variable * v)
{
  int res, sign;

  if ((sign = is_signed (v)))
    v = not (v);
  res = v->score[sign];

  return res;
}

/*------------------------------------------------------------------------*/
/* We want to have a rather stable sorting criteria for variables.  In
 * particular the decision process should not depend on the permutation of
 * clauses in the file.
 */
inline static int
cmp (Limmat * limmat, Variable * v0, Variable * v1)
{
  int score[2], id[2], res;

  assert (v0 != FALSE);
  assert (v0 != TRUE);
  assert (v1 != FALSE);
  assert (v1 != TRUE);

  if (limmat && limmat->stats)
    limmat->stats->compared++;

  if (v0 == v1)
    res = 0;
  else
    {
      score[0] = get_score (v0);
      score[1] = get_score (v1);

      /* The first sorting criteria is the 'score'.
       */
      if (score[0] < score[1])
	res = -1;
      else if (score[0] > score[1])
	res = 1;
      else if (v0 == not (v1))
	{
	  /* Unsigned variables with the same score and id are smaller.
	   */
	  res = (v0 < v1) ? -1 : 1;
	}
      else
	{
	  /* Now even the 'score' of the two variables is the same. Then use
	   * the 'id' of the variables as sorting criteria.  Variables with
	   * smaller indices are supposed to be larger.  Since the id of the
	   * two variables can not be same, because of the last and the
	   * first test, we can just forget about the sign.
	   */
	  v0 = strip (v0);
	  v1 = strip (v1);
	  id[0] = v0->id;
	  id[1] = v1->id;
	  assert (id[0] != id[1]);
	  res = (id[0] < id[1]) ? 1 : -1;
	}
    }

  return res;
}

/*------------------------------------------------------------------------*/
#ifdef EXPENSIVE_CHECKS
/*------------------------------------------------------------------------*/

static Clause *
find_empty_clause (Limmat * limmat)
{
  Clause *clause, **p, **eoc, *res;
  int i, num_literals, found;

  res = 0;

  forall_clauses (limmat, p, eoc)
  {
    clause = *p;
    num_literals = clause->size;
    found = 0;

    for (i = 0; !found && i < num_literals; i++)
      found =
	(deref (lit2var (clause2literals (limmat, clause)[i])) != FALSE);

    if (!found)
      res = clause;
  }

  return res;
}

/*------------------------------------------------------------------------*/

static void
check_no_empty_clause (Limmat * limmat)
{
#if 0
#warning "check_no_empty_clause disabled"
#else
  Clause *clause;

  if (!check_it_now (limmat))
    return;

  clause = find_empty_clause (limmat);
  assert (!clause);
#endif
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Unassign all variables assigned at 'decision_level' and higher.
 */
static void
untrail (Limmat * limmat, int decision_level)
{
  Variable **all, **start, **p, *src;
  int lower_pos, new_trail_level;
  Frame *frame;

  assert (decision_level <= limmat->decision_level);

  all = start_of_Stack (&limmat->trail);
  p = all + count_Stack (&limmat->trail);

  if (decision_level >= 0)
    {
      frame = limmat->control + decision_level;
      new_trail_level = frame->trail_level;
    }
  else
    new_trail_level = 0;

  start = all + new_trail_level;
  assert (p >= start);

  while (p > start)
    {
      src = *--p;

      assert (!is_signed (src));
      assert (is_assigned (src));
      assert (src->decision_level >= decision_level);

      src->assignment = src;
      src->reason = 0;
      src->decision_level = -1;

      assert (limmat->num_assigned > 0);
      limmat->num_assigned--;

      lower_pos = min (src->pos[0], src->pos[1]);
      if (lower_pos <= limmat->last_assigned_pos)
	limmat->last_assigned_pos = lower_pos - 1;
    }

  reset_Stack (limmat, &limmat->trail, new_trail_level);

#ifdef EXPENSIVE_CHECKS
  check_no_empty_clause (limmat);
#endif
}

/*------------------------------------------------------------------------*/

static void
reset_control (Limmat * limmat)
{
  untrail (limmat, -1);
  limmat->decision_level = -1;
}

/*------------------------------------------------------------------------*/

static void
reset_clauses (Limmat * limmat)
{
  Clause *clause, **p, **eoc;

  forall_clauses (limmat, p, eoc)
  {
    clause = *p;
    delete (limmat, clause, sizeof (Clause));
  }

  reset_Stack (limmat, &limmat->clauses, 0);
  limmat->contains_empty_clause = 0;
}

/*------------------------------------------------------------------------*/

static void
reset_variables (Limmat * limmat)
{
  Variable *v, *end;

  forall_variables (limmat, v, end)
  {
    release_Stack (limmat, &v->watched[0]);
    release_Stack (limmat, &v->watched[1]);
  }

  reset_Arena (limmat, &limmat->variables);
}

/*------------------------------------------------------------------------*/

static void
reset_order (Limmat * limmat)
{
  reset_Stack (limmat, &limmat->order, 0);
  limmat->last_assigned_pos = -1;
  limmat->first_zero_pos = 0;
}

/*------------------------------------------------------------------------*/

static void
reset_error (Limmat * limmat)
{
  if (limmat->error)
    {
      delete (limmat, limmat->error, strlen (limmat->error) + 1);
      limmat->error = 0;
    }
}

/*------------------------------------------------------------------------*/

static void
reset_Limmat (Limmat * limmat)
{
  reset_assignments (limmat);
  reset_Stack (limmat, &limmat->clause, 0);
  reset_control (limmat);
  reset_clauses (limmat);
  reset_order (limmat);
  reset_variables (limmat);
  reset_error (limmat);
}

/*------------------------------------------------------------------------*/

static void
release_assignments (Limmat * limmat)
{
  int bytes;

  bytes = sizeof (Assignment) * limmat->size_assignments;
  delete (limmat, limmat->assignments, bytes);
}

/*------------------------------------------------------------------------*/

static void
release_control (Limmat * limmat)
{
  int bytes;

  bytes = sizeof (Frame) * limmat->control_size;
  delete (limmat, limmat->control, bytes);

  release_Stack (limmat, &limmat->trail);
}

/*------------------------------------------------------------------------*/

static void
release_statistics (Limmat * limmat)
{
  if (limmat->stats)
    delete (limmat, limmat->stats, sizeof (Statistics));
}

/*------------------------------------------------------------------------*/

static void
release_Limmat (Limmat * limmat)
{
  release_assignments (limmat);
  release_Stack (limmat, &limmat->stack);
  release_Stack (limmat, &limmat->clause);
  release_control (limmat);
  release_Stack (limmat, &limmat->clauses);
  release_Stack (limmat, &limmat->units);
  release_Stack (limmat, &limmat->literals);
  release_Stack (limmat, &limmat->order);
  release_Arena (limmat, &limmat->variables);
  release_statistics (limmat);
}

/*------------------------------------------------------------------------*/

static int
internal_delete_Limmat (Limmat * limmat)
{
  int res;

  reset_Limmat (limmat);
  release_Limmat (limmat);
  res = limmat->bytes;
  free (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

void
delete_Limmat (Limmat * limmat)
{
  (void) internal_delete_Limmat (limmat);
}

/*------------------------------------------------------------------------*/

static void
parse_error (Parser * parser, const char *str1, const char *str2)
{
  const char *p;
  int i;

  if (parser->error)
    delete (parser->limmat, parser->error, ERROR_SIZE);
  parser->error = (char *) new (parser->limmat, ERROR_SIZE);

  i = 0;

  if (parser->name)
    {
      for (p = parser->name; *p && i < ERROR_SIZE - 2; i++, p++)
	parser->error[i] = *p;

      parser->error[i++] = ':';
    }

  if (i < ERROR_SIZE - 30)
    {
      sprintf (parser->error + i, "%d: ", parser->lineno);
      i = strlen (parser->error);
      for (p = str1; *p && i < ERROR_SIZE - 1; i++, p++)
	parser->error[i] = *p;

      if (str2)
	{
	  for (p = str2; *p && i < ERROR_SIZE - 1; i++, p++)
	    parser->error[i] = *p;
	}
    }

  assert (i < ERROR_SIZE);

  parser->error[i] = 0;
}

/*------------------------------------------------------------------------*/

static Parser *
new_Parser (Limmat * limmat, FILE * file, const char *input_name)
{
  const char *name;
  Parser *res;

  res = (Parser *) new (limmat, sizeof (Parser));
  res->num_specified_clauses = -1;
  res->num_specified_max_id = -1;
  res->limmat = limmat;
  init_Stack (limmat, &res->literals);
  res->lineno = 0;
  res->error = 0;

  if (input_name || file)
    name = input_name;
  else
    name = "<stdin>";

  res->name = 0;

  if (name)
    {
      res->name = (char *) new (limmat, strlen (name) + 1);
      strcpy (res->name, name);
    }

  res->close_file = 0;

  if (file)
    res->file = file;
  else if (input_name)
    {
      res->file = fopen (name, "r");
      if (!res->file)
	parse_error (res, "could not open file", 0);
      else
	res->close_file = 1;
    }
  else
    res->file = stdin;

  res->lineno = 1;

  return res;
}

/*------------------------------------------------------------------------*/

static void
delete_Parser (Parser * parser)
{
  if (parser->close_file)
    fclose (parser->file);

  if (parser->name)
    delete (parser->limmat, parser->name, strlen (parser->name) + 1);

  if (parser->error)
    delete (parser->limmat, parser->error, ERROR_SIZE);

  release_Stack (parser->limmat, &parser->literals);
  delete (parser->limmat, parser, sizeof (Parser));
}

/*------------------------------------------------------------------------*/

static int
read_number (Parser * parser, int *literal)
{
  int res, tmp, ch;

  ch = getc (parser->file);

  if (isdigit (ch))
    {
      tmp = ch - '0';
      res = 1;

      while (isdigit (ch = getc (parser->file)))
	tmp = 10 * tmp + (ch - '0');

      *literal = tmp;
      if (ch != EOF)
	ungetc (ch, parser->file);
    }
  else
    {
      parse_error (parser, "expected digit", 0);
      res = 0;
    }

  return res;
}

/*------------------------------------------------------------------------*/

static int
read_white_space (Parser * parser)
{
  int res, ch;

  ch = getc (parser->file);
  if (isspace (ch))
    {
      while (isspace (ch = getc (parser->file)))
	;

      if (ch != EOF)
	ungetc (ch, parser->file);

      res = 1;
    }
  else
    {
      parse_error (parser, "expected white space", 0);
      res = 0;
    }

  return res;
}

/*------------------------------------------------------------------------*/

static int
read_clause (Parser * parser)
{
  int ch, literal, sign, res, found_zero;

  if (parser->error)
    return 0;

  assert (parser->file);

  reset_Stack (parser->limmat, &parser->literals, 0);
  found_zero = 0;
  res = 1;

  do
    {
      while (isspace (ch = getc (parser->file)))
	if (ch == '\n')
	  parser->lineno++;

      sign = 1;

      switch (ch)
	{
	case '-':
	  sign = -1;
	  /* FALL THROUGH */
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (ch != '-')
	    ungetc (ch, parser->file);
	  res = read_number (parser, &literal);
	  literal *= sign;
	  if (res)
	    {
	      if (!literal)
		found_zero = 1;
	      else
		push (parser->limmat, &parser->literals,
		      (void *) (PTR_SIZED_WORD) literal);
	    }
	  break;

	case 'c':
	  while ((ch = getc (parser->file)) != '\n' && ch != EOF)
	    ;
	  if (ch == '\n')
	    parser->lineno++;
	  break;

	case 'p':
	  if (parser->num_specified_clauses >= 0)
	    {
	      parse_error (parser, "found 'p' spec twice", 0);
	      res = 0;
	    }
	  else if (read_white_space (parser))
	    {
	      if (getc (parser->file) != 'c' ||
		  getc (parser->file) != 'n' || getc (parser->file) != 'f')
		{
		  parse_error (parser, "expected 'cnf' after 'p'", 0);
		  res = 0;
		}
	      else if (read_white_space (parser))
		{
		  if (read_number (parser, &parser->num_specified_max_id))
		    {
		      if (read_white_space (parser))
			{
			  res = read_number (parser,
					     &parser->num_specified_clauses);
			}
		      else
			res = 0;
		    }
		  else
		    res = 0;
		}
	      else
		res = 0;
	    }
	  else
	    res = 0;
	  break;

	case EOF:
	  res = 0;
	  break;

	default:
	  parse_error (parser, "non valid character", 0);
	  res = 0;
	  break;
	}
    }
  while (res && !found_zero && ch != EOF);

  if (ch == EOF)
    {
      if (!found_zero && count_Stack (&parser->literals))
	{
	  parse_error (parser, "expected '0' after clause", 0);
	  res = 0;
	}
    }
  else if (!res)
    assert (parser->error);

  return res;
}

/*------------------------------------------------------------------------*/

inline static void
inc_ref (Limmat * limmat, Variable * v)
{
  Variable *stripped;
  int sign;

  stripped = (sign = is_signed (v)) ? not (v) : v;
  stripped->ref[sign]++;
}

/*------------------------------------------------------------------------*/

inline static int
var2int (Variable * v)
{
  int sign, res;

  if ((sign = is_signed (v)))
    v = not (v);
  res = v->id;
  if (sign)
    res = -res;

  return res;
}

/*------------------------------------------------------------------------*/

inline static void
init_Variable (Limmat * limmat, Variable * v, int id)
{
  int i;

  v->id = id;
  v->decision_level = -1;

  for (i = 0; i < 2; i++)
    {
      v->score[i] = 0;
      v->ref[i] = 0;
      v->saved[i] = 0;
      init_Stack (limmat, &v->watched[i]);
      v->pos[i] = 0;
    }

  v->assignment = v;
  v->reason = 0;

  v->pos_on_queue = -1;
  v->sign_on_queue = 0;

  v->sign_in_clause = 0;
  v->mark = 0;
}

/*------------------------------------------------------------------------*/
#ifndef NDEBUG
/*------------------------------------------------------------------------*/

inline static int
get_pos (Variable * v)
{
  int sign, res;

  sign = is_signed (v);
  if (sign)
    v = not (v);
  res = v->pos[sign];

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

inline static void
set_pos (Variable * v, int pos)
{
  int sign;

  sign = is_signed (v);
  if (sign)
    v = not (v);
  v->pos[sign] = pos;
}

/*------------------------------------------------------------------------*/

inline static void
insert_order (Limmat * limmat, Variable * v)
{
  /* Since we assume that an unsigned variables is smaller than the same
   * signed variable and the larger variables should occur before smaller in
   * the order, we put 'not(v)' before 'v' in the order.
   */
  assert (cmp (0, v, not (v)) < 0);

  set_pos (not (v), count_Stack (&limmat->order));
  push (limmat, &limmat->order, not (v));

  set_pos (v, count_Stack (&limmat->order));
  push (limmat, &limmat->order, v);

  /* Otherwise we have to update 'limmat->first_zero_pos'.
   */
  assert (!v->score[0]);
  assert (!v->score[1]);
}

/*------------------------------------------------------------------------*/

inline static Variable *
find (Limmat * l, int id)
{
  int sign, fresh;
  Variable *res;
  size_t delta;

  assert (id);

  sign = (id < 0);
  if (sign)
    id = -id;

  res = access_Arena (l, &l->variables, id, sizeof (Variable), &delta);
  fresh = !res->id;

  if (fresh)
    {
      init_Variable (l, res, id);
      if (delta)
	fix_pointer (&res->assignment, -delta);
      l->num_variables++;
      if (l->max_id < id)
	l->max_id = id;
    }

  if (delta)
    fix_pointers_to_variables (l, delta);
  if (fresh)
    insert_order (l, res);
  if (sign)
    res = not (res);

  assert (is_aligned_ptr (strip (res), VARIABLE_ALIGNMENT));

  return res;
}

/*------------------------------------------------------------------------*/

static Stack *
var2stack (Variable * v)
{
  Stack *res;
  int sign;

  if ((sign = is_signed (v)))
    v = not (v);
  res = &v->watched[sign];

  return res;
}

/*------------------------------------------------------------------------*/

static int *
clause2watched (Clause * c)
{
  int *res, sign;

  sign = is_signed (c);
  if (sign)
    c = not (c);

  res = &c->watched[sign];

  return res;
}

/*------------------------------------------------------------------------*/

inline static void
connect (Limmat * limmat, Clause * clause, int l)
{
  Variable *v, **literals;
  Occurence occurrence;
  Clause *stripped;
  int vs;

  /* Get the corresponding variable and its sign at position 'l' in 'clause'.
   */
  int *watched;

  stripped = strip (clause);

  assert (stripped->size > l);
  literals = clause2literals (limmat, stripped);
  v = lit2var (literals[l]);
  if ((vs = is_signed (v)))
    v = not (v);

  /* Mark this literal as watched in the clause.
   */
  watched = clause2watched (clause);
  assert (*watched < 0);	/* is inwatched */
  *watched = l;
  occurrence = clause;

  /* Finally enqueue this occurence to the variable.
   */
  push (limmat, &v->watched[vs], occurrence);

#ifdef LOG_CONNECT
  fprintf (LOGFILE (limmat),
	   "CONNECT\t%d\t%u\n",
	   var2int (lit2var (literals[l])), count_Stack (&v->watched[vs]));
#endif
}

/*------------------------------------------------------------------------*/

static void
partial_disconnect (Limmat * limmat, Clause * clause, int l)
{
  Variable **literals;
  Clause *stripped;
  int *watched;

  /* Mark the literal as watched in the clause.
   */
  watched = clause2watched (clause);
  assert (*watched == l);
  stripped = strip (clause);
  assert (stripped->size > l);
  literals = clause2literals (limmat, stripped);
  *watched = -1;
}

/*------------------------------------------------------------------------*/
#if defined(CHECK_CONSISTENCY) && defined(EXPENSIVE_CHECKS) && !defined(NDEBUG)
/*------------------------------------------------------------------------*/

static int
is_connected (Variable * v, Clause * clause)
{
  Clause **p, **end;
  int res;

  res = 0;

  forall_Stack (var2stack (v), Clause *, p, end) if ((res = (*p == clause)))
    break;

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

static void
invariant_clause (Limmat * limmat, Clause * clause)
{
#ifdef CHECK_CONSISTENCY
  Variable *v, **literals, *tmp, *l[2], *d[2], *s[2];
  int i, num_non_false, p[2];

  assert (clause->size >= 1);

  if (!check_it_now (limmat))
    return;

  p[0] = clause->watched[0];
  p[1] = clause->watched[1];

  literals = clause2literals (limmat, clause);

  if (clause->size > 1)
    {
      assert (p[0] >= 0);
      assert (p[1] >= 0);
      assert (p[0] < clause->size);
      assert (p[1] < clause->size);
      assert (p[0] != p[1]);

      num_non_false = 0;
      for (i = 0; i < clause->size; i++)
	{
	  v = lit2var (literals[i]);
	  tmp = deref (v);
	  if (tmp != FALSE)
	    num_non_false++;
	}

      l[0] = lit2var (literals[p[0]]);
      l[1] = lit2var (literals[p[1]]);
      d[0] = deref (l[0]);
      d[1] = deref (l[1]);
      s[0] = strip (l[0]);
      s[1] = strip (l[1]);

      if (num_non_false >= 2)
	{
	  if (d[0] == FALSE)
	    {
	      assert (d[1] == TRUE);
	      assert (s[0]->decision_level >= s[1]->decision_level);
	    }

	  if (d[1] == FALSE)
	    {
	      assert (d[0] == TRUE);
	      assert (s[1]->decision_level >= s[0]->decision_level);
	    }

#ifdef EXPENSIVE_CHECKS
	  assert (is_connected (l[0], clause));
	  assert (is_connected (l[1], not (clause)));
#endif
	}
      else if (num_non_false == 1)
	{
#ifdef EXPENSIVE_CHECKS
	  assert (is_connected (l[0], clause));
#endif

	  assert ((d[0] == FALSE) + (d[1] == FALSE) == 1);
	}

      for (i = 0; i < clause->size; i++)
	{
	  v = strip (lit2var (literals[i]));

	  if (i != p[0] && i != p[1])
	    {
	      if (d[0] == FALSE && d[1] != TRUE)
		assert (v->decision_level <= s[0]->decision_level);

	      if (d[1] == FALSE && d[0] != TRUE)
		assert (v->decision_level <= s[1]->decision_level);
	    }
	}
    }
  else
    {
      assert (p[0] == 0);
      assert (p[1] == -1);
    }
#endif
}

/*------------------------------------------------------------------------*/

static void
invariant (Limmat * limmat)
{
#ifdef EXPENSIVE_CHECKS
  Variable *v, *eov, *u, **order;
  Clause **p, **eoc, *clause;
  int i, score, *watched;


  if (!check_it_now (limmat))
    return;

  assert (limmat->current_max_score <= limmat->max_score);

  assert (limmat->num_assigned <= limmat->num_variables);

  forall_clauses (limmat, p, eoc) invariant_clause (limmat, *p);

  order = start_of_Stack (&limmat->order);

  forall_variables (limmat, v, eov)
  {
    for (i = 0; i < 2; i++)
      {
	u = (i ? not (v) : v);

	assert (order[get_pos (u)] == u);

	if (!is_assigned (u))
	  assert (limmat->last_assigned_pos < get_pos (u));

	score = get_score (u);
	assert (score >= 0);
	if (score)
	  assert (limmat->first_zero_pos > get_pos (u));

	forall_Stack (&v->watched[i], Clause *, p, eoc)
	{
	  watched = clause2watched (*p);
	  clause = strip (*p);
	  assert (lit2var (clause2literals (limmat, clause)[*watched]) == u);
	}
      }
  }
#endif
}

/*------------------------------------------------------------------------*/
#if defined(EXPENSIVE_CHECKS) && !defined(NDEBUG)
/*------------------------------------------------------------------------*/

static int
is_unique_clause (Limmat * limmat, Clause * clause)
{
  Variable *u, *v;
  int i, j, res;

  for (i = 0, res = 1; res && i < clause->size - 1; i++)
    {
      u = strip (lit2var (clause2literals (limmat, clause)[i]));
      for (j = i + 1; res && j < clause->size; j++)
	{
	  v = strip (lit2var (clause2literals (limmat, clause)[j]));
	  res = (u->id != v->id);
	}
    }

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Add a new clause to the clause data base.  The pointers to the two
 * watched literals in the clause have are set as well.  In case
 * 'conflict_driven_assignment' is non-zero generate a conflict driven
 * assignment driven by this clause.
 */
static Clause *
add_clause (Limmat * limmat,
	    Variable ** variables,
	    int n, Assignment * conflict_driven_assignment)
{
  int i, watched[2], max[2], tmp, found_non_false[2], level;
  Variable *v, *stripped_literal;
  Statistics *stats;
  Clause *clause;

  assert (n > 0);

  /* In the first part we just adjust the statistics.
   */
  limmat->added_clauses++;
  limmat->num_clauses++;

  stats = limmat->stats;

  if (stats)
    {
      if (stats->max_clauses < limmat->num_clauses)
	stats->max_clauses = limmat->num_clauses;
    }

  limmat->added_literals += n;
  limmat->num_literals += n;

  if (stats)
    {
      if (stats->max_literals < limmat->num_literals)
	stats->max_literals = limmat->num_literals;
    }

  /* Then we allocate the clause and initialize some of its components.
   */
  clause = (Clause *) new (limmat, sizeof (Clause));
  clause->id = count_Stack (&limmat->clauses);
  push (limmat, &limmat->clauses, clause);
  clause->learned = 0;
  clause->size = n;

  clause->watched[0] = -1;
  clause->watched[1] = -1;
#ifdef OPTIMIZE_DIRECTION
  clause->direction = 1;
#endif

  clause->literals_idx = count_Stack (&limmat->literals);

  /* Push first literal of clause onto literals stack.
   */
  v = variables[0];
  push (limmat, &limmat->literals, v);

  /* Push inner literals.
   */
  for (i = 1; i < n - 1; i++)
    {
      v = variables[i];
      push (limmat, &limmat->literals, v);
    }

  /* Push last literal.
   */
  if (n > 1)
    {
      v = variables[n - 1];
      push (limmat, &limmat->literals, v);
    }

  /* Finally we have to find two literals that are assigned at the largest
   * decision level or are still unassigned.  We start with the first
   * literal.
   */
  found_non_false[0] = 0;
  watched[0] = -1;
  max[0] = -2;			/* unassigned decision_level is '-1' */

  for (i = 0; !found_non_false[0] && i < n; i++)
    {
      v = variables[i];

      if (deref (v) == FALSE)
	{
	  tmp = ((Variable *) strip (v))->decision_level;
	  if (tmp > max[0])
	    {
	      watched[0] = i;
	      max[0] = tmp;
	    }
	}
      else
	{
	  found_non_false[0] = 1;
	  watched[0] = i;
	}
    }

  /* Connect the first pointer to the largest assigned or still unassigned
   * literal.
   */
  assert (watched[0] >= 0);
  connect (limmat, clause, watched[0]);

  /* This previously choosen literal will necessarily be also the conflict
   * driven assignment in case we want to generate one.
   */
  if (conflict_driven_assignment)
    {
      conflict_driven_assignment->literal = variables[watched[0]];
      conflict_driven_assignment->reason = clause;
      conflict_driven_assignment->decision_level = -1;
    }

  /* The decision level of the conflict driven assignment still need to be
   * calculated as the maximal level of all assigned variables beside the
   * first found literal.
   */
  level = -1;

  if (n > 1)
    {
      watched[1] = -1;
      found_non_false[1] = 0;
      max[1] = -2;		/* unassigned decision_level is '-1' */

      for (i = 0; !found_non_false[1] && i < n; i++)
	{
	  if (i != watched[0])	/* skip the first watched literal */
	    {
	      v = variables[i];

	      if (deref (v) == FALSE)
		{
		  tmp = ((Variable *) strip (v))->decision_level;
		  if (tmp > max[1])
		    {
		      watched[1] = i;
		      max[1] = tmp;
		    }
		}
	      else
		{
		  found_non_false[1] = 1;
		  watched[1] = i;
		}
	    }
	}

      /* Connect the second watched literal.
       */
      assert (watched[1] >= 0);
      connect (limmat, not (clause), watched[1]);

      if (conflict_driven_assignment)
	{
	  /* The level of the conflict driven assignment is actually
	   * determined by the decision level of the second literal.
	   */
	  assert (max[1] >= -1);
	  level = max[1];
	}
    }
  else
    {
      push (limmat, &limmat->units, clause);

#ifdef LOG_UNIT
      if (!limmat->dont_log)
	fprintf (LOGFILE (limmat), "UNIT\t%d\n", var2int (variables[0]));
#endif
    }

  if (conflict_driven_assignment)
    {
      conflict_driven_assignment->decision_level = level;

      if (stats && n > 1)
	{
	  stripped_literal = strip (conflict_driven_assignment->literal);

	  for (i = 0; i < n; i++)
	    {
	      v = strip (variables[i]);

	      if (v != stripped_literal)
		{
		  assert (v->decision_level <= level);
		}
	    }
	}
    }

  for (i = 0; i < n; i++)
    inc_ref (limmat, variables[i]);

  invariant_clause (limmat, clause);

#ifdef EXPENSIVE_CHECKS
  if (check_it_now (limmat))
    assert (is_unique_clause (limmat, clause));
#endif

  return clause;
}

/*------------------------------------------------------------------------*/

static void
print_clause (FILE * file, Limmat * limmat, Clause * clause)
{
  int i;

#ifndef NDEBUG
  if (!file)
    file = stdout;
#endif

  for (i = 0; i < clause->size; i++)
    fprintf (file, "%d ",
	     var2int (lit2var (clause2literals (limmat, clause)[i])));

  fprintf (file, "0\n");
}

/*------------------------------------------------------------------------*/

static int
real_max_id (Limmat * limmat)
{
  Clause **eoc, **p, *clause;
  int res, i, num_literals;
  Variable *v;

  res = 0;

  forall_clauses (limmat, p, eoc)
  {
    clause = *p;
    num_literals = clause->size;

    for (i = 0; i < num_literals; i++)
      {
	v = strip (lit2var (clause2literals (limmat, clause)[i]));
	if (v->id > res)
	  res = v->id;
      }
  }

  return res;
}

/*------------------------------------------------------------------------*/

void
print_Limmat (Limmat * limmat, FILE * file)
{
  Clause **p, **eoc;
  double delta;

  start_timer (&delta);

  if (!file)
    file = stdout;

  if (limmat->error)
    {
      fprintf (file, "c %s\n", limmat->error);
    }
  else if (limmat->contains_empty_clause)
    {
      fprintf (file, "p cnf 0 1\n0\n");
    }
  else
    {
      fprintf (file, "p cnf %d %d\n",
	       real_max_id (limmat), count_Stack (&limmat->clauses));

      forall_clauses (limmat, p, eoc) print_clause (file, limmat, *p);
    }

  limmat->time += stop_timer (delta);
}

/*------------------------------------------------------------------------*/
#if !defined(NDEBUG) || defined(LOG_SOMETHING)
/*------------------------------------------------------------------------*/

void
dump_clause_aux (FILE * file,
		 Limmat * limmat,
		 Clause * clause,
		 Variable * exception,
		 int negate_literals,
		 int print_level, int print_assignment, int print_connection)
{
  Variable *v, *tmp, *stripped, **literals, *l;
  int i;

  if (!file)
    file = stdout;

  if (clause)
    {
      literals = clause2literals (limmat, clause);

      for (i = 0; i < clause->size; i++)
	{
	  l = literals[i];
	  v = lit2var (l);

	  if (v != exception)
	    {
	      if (negate_literals)
		v = not (v);

	      stripped = strip (v);

	      if (print_connection)
		{
		  if (clause->watched[0] == i || clause->watched[1] == i)
		    fputc ('>', file);
		}

	      fprintf (file, "%d", var2int (v));

	      if (print_level)
		fprintf (file, "@%d", stripped->decision_level);

	      if (print_assignment && is_assigned (v))
		{
		  tmp = deref (v);
		  fputs (tmp == FALSE ? "=0" : "=1", file);
		}

	      if (i < clause->size - 1)
		fputc (' ', file);
	    }
	}
    }

  fputc ('\n', file);
}

/*------------------------------------------------------------------------*/

void
dump_clause (Limmat * limmat, Clause * clause)
{
  dump_clause_aux (stdout, limmat, clause, 0, 0, 1, 1, 1);
}

/*------------------------------------------------------------------------*/

void
dump (Limmat * limmat)
{
  Clause **p, **eoc;

  forall_clauses (limmat, p, eoc) dump_clause (limmat, *p);
}

/*------------------------------------------------------------------------*/

static void
dump_frame (Frame * frame)
{
  int id;

  id = var2int (frame->decision);
  printf ("decision level %d: %d\n", frame->decision_level, id);
}

/*------------------------------------------------------------------------*/

void
dump_control (Limmat * limmat)
{
  Frame *frame;
  int i;

  for (i = 0; i <= limmat->decision_level; i++)
    {
      frame = limmat->control + i;
      dump_frame (frame);
    }
}

/*------------------------------------------------------------------------*/

#define ASSIGNED_MSG "------------------ assigned variables\n"
#define ZERO_MSG "------------------ zero score variables\n"

/*------------------------------------------------------------------------*/

void
dump_order (Limmat * limmat)
{
  Variable **eoo, **p;
  int i;

  i = 0;

  forall_Stack (&limmat->order, Variable *, p, eoo)
  {
    if (i - 1 == limmat->last_assigned_pos)
      printf (ASSIGNED_MSG);

    printf ("%d %d %d\n", i++, var2int (*p), get_score (*p));

    if (i == limmat->first_zero_pos)
      printf (ZERO_MSG);
  }
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Find number of unique literals and rearrange 'literals' to have the
 * unique literals at the beginning of the array.  In case a literal and its
 * negation are found the result is negative to denote that the clause is
 * trivial .
 */
static int
unique_literals (Variable ** literals, int n)
{
  Variable *u, *stripped;
  int i, res, sign;

  for (i = 0, res = 0; res >= 0 && i < n; i++)
    {
      assert (res <= i);

      u = literals[i];
      sign = is_signed (u);
      stripped = sign ? not (u) : u;

      if (stripped->mark)
	{
	  /* The variable 'stripped' occurs again in the clause.  If it
	   * occured with the opposite sign, the clause is trivial and '-1'
	   * is returned.  Otherwise we can skip the second occurence.
	   */
	  if (!sign != !stripped->sign_in_clause)
	    res = -1;
	}
      else
	{
	  literals[res++] = u;
	  stripped->sign_in_clause = sign ? 1 : 0;
	  stripped->mark = 1;
	}
    }

  while (i > 0)
    {
      stripped = strip (literals[--i]);
      stripped->sign_in_clause = 0;
      stripped->mark = 0;
    }

  return res;
}

/*------------------------------------------------------------------------*/

void
add_Limmat (Limmat * limmat, const int *ivec)
{
  int i, n, max, tmp, old_literals_count;
  Variable **vvec, *v;

  assert (ivec);

  old_literals_count = count_Stack (&limmat->clause);

  /* First find the the maximal id of a variable int the new clause.
   */
  for (n = 0, max = 0; ivec[n]; n++)
    {
      tmp = ivec[n];
      if (tmp < 0)
	tmp = -tmp;
      if (tmp > max)
	max = tmp;
    }

  if (n)
    {
      /* Then access the variable with the largest index first, such that
       * further access to variables in this clause do not require resizing
       * the variables arena, which would result in invalidating the
       * previously generated variable pointers.
       */
      assert (max > 0);
      (void) find (limmat, max);

      /* Now we can savely generate variable pointers in turn, since the
       * variable arena should already be large enough to hold all variables
       * of this clause and will not move.
       */
      for (i = 0; i < n; i++)
	{
	  v = find (limmat, ivec[i]);
	  push (limmat, &limmat->clause, v);
	}

      vvec = (Variable **) start_of_Stack (&limmat->clause);
      vvec += old_literals_count;
      n = unique_literals (vvec, n);
      assert (n != 0);

      if (n > 0)
	add_clause (limmat, vvec, n, 0);
    }
  else
    {
      reset_Limmat (limmat);
      limmat->contains_empty_clause = 1;
    }

  reset_Stack (limmat, &limmat->clause, old_literals_count);
}

/*------------------------------------------------------------------------*/

int
read_Limmat (Limmat * limmat, FILE * file, const char *name)
{
  int res, n, num_literals, num_clauses;
  Parser *parser;

  start_timer (&limmat->timer);
  parser = new_Parser (limmat, file, name);

  num_clauses = 0;
  num_literals = 0;

  while (read_clause (parser) && !limmat->contains_empty_clause)
    {
      n = count_Stack (&parser->literals);
      num_clauses++;
      num_literals += n;
      push (limmat, &parser->literals, (void *) (int) 0);
      add_Limmat (limmat, intify_Stack (&parser->literals));
    }

  if (!parser->error &&
      !limmat->contains_empty_clause && parser->num_specified_clauses >= 0)
    {
      assert (parser->num_specified_max_id >= 0);

      if (parser->num_specified_clauses != num_clauses)
	{
	  parse_error (parser,
		       "number of specified clauses does not match", 0);
	}
      else if (parser->num_specified_max_id < limmat->max_id)
	{
	  parse_error (parser, "specified maximal variable exceeded", 0);
	}
    }

  if (parser->error)
    {
      reset_Limmat (limmat);
      limmat->error = (char *) new (limmat, strlen (parser->error) + 1);
      strcpy (limmat->error, parser->error);
      res = 0;
    }
  else
    {
      if (limmat->stats)
	{
	  limmat->stats->original_clauses += num_clauses;
	  limmat->stats->original_literals += num_literals;
	}

      res = 1;
    }

  delete_Parser (parser);
  invariant (limmat);
  limmat->time += stop_timer (limmat->timer);

  return res;
}

/*------------------------------------------------------------------------*/

const char *
error_Limmat (Limmat * limmat)
{
  return limmat->error;
}

/*------------------------------------------------------------------------*/

int
maxvar_Limmat (Limmat * limmat)
{
  return limmat->max_id;
}

/*------------------------------------------------------------------------*/

int
literals_Limmat (Limmat * limmat)
{
  return limmat->num_literals;
}

/*------------------------------------------------------------------------*/

int
clauses_Limmat (Limmat * limmat)
{
  return limmat->num_clauses;
}

/*------------------------------------------------------------------------*/

double
time_Limmat (Limmat * limmat)
{
  return limmat->time;
}

/*------------------------------------------------------------------------*/

size_t
bytes_Limmat (Limmat * limmat)
{
  return limmat->bytes;
}

/*------------------------------------------------------------------------*/

static void
stats_performance (const char *description,
		   double absolute_value, double time, FILE * file)
{
  double performance;

  if (time)
    performance = absolute_value / time;
  else
    performance = 0;

  fprintf (file,
	   LIMMAT_PREFIX
	   "%.0f %s (%.0f/sec)\n", absolute_value, description, performance);
}

/*------------------------------------------------------------------------*/

void
stats_Limmat (Limmat * limmat, FILE * file)
{
  double avg_num, avg_original, avg_learned, avg_max, avg_added, avg_removed;
  double avg_rescored_variables, avg_assigned_in_decision;
  double bytes, H, M, S, MB, KB;
  double avg_score_in_decision;
  Statistics *stats;
  int round;

  start_timer (&limmat->timer);

  stats = limmat->stats;

  S = limmat->time;
  H = S / 3600;
  round = H;
  H = round;
  M = (S - 3600 * H) / 60;
  round = M;
  M = round;
  S = S - 60 * M;

  fprintf (file,
	   LIMMAT_PREFIX
	   "%.2f seconds in library (%.0f:%02.0f:%05.2f)\n",
	   limmat->time, H, M, S);

  bytes = limmat->max_bytes + sizeof (Limmat);

  fprintf (file, LIMMAT_PREFIX "%.0f bytes memory usage", bytes);

  KB = 1024;
  MB = KB * KB;

  if (bytes >= KB / 10)
    {
      if (bytes >= MB / 10)
	fprintf (file, " (%.1f MB)", bytes / MB);
      else
	fprintf (file, " (%.1f KB)", bytes / KB);
    }

  fputc ('\n', file);

  if (!limmat->num_clauses)
    avg_num = 0;
  else
    avg_num = limmat->num_literals / (double) limmat->num_clauses;

  if (stats)
    {
      fputs (LIMMAT_PREFIX "\n", file);

      stats_performance ("conflicts", limmat->num_conflicts, limmat->time,
			 file);
      stats_performance ("decisions", limmat->num_decisions, limmat->time,
			 file);
      stats_performance ("assignments", stats->assignments, limmat->time,
			 file);
      stats_performance ("propagations", stats->propagations, limmat->time,
			 file);
      stats_performance ("visits", stats->visits, limmat->time, file);
      stats_performance ("compared", stats->compared, limmat->time, file);
      stats_performance ("swapped", stats->swapped, limmat->time, file);
      stats_performance ("searched", stats->searched, limmat->time, file);

      fputs (LIMMAT_PREFIX "\n", file);

      fprintf (file,
	       LIMMAT_PREFIX
	       "%.0f restarts, %.0f rescores\n",
	       limmat->restart.finished, limmat->rescore.finished);

      fprintf (file,
	       LIMMAT_PREFIX "%.0f uips, %.0f backjumps\n",
	       stats->uips, stats->backjumps);

      fputs (LIMMAT_PREFIX "\n", file);

      if (limmat->rescore.finished)
	{
	  avg_rescored_variables =
	    stats->rescored_variables / limmat->rescore.finished;
	}
      else
	avg_rescored_variables = 0;

      if (limmat->num_decisions)
	{
	  avg_assigned_in_decision =
	    stats->sum_assigned_in_decision / limmat->num_decisions;

	  avg_score_in_decision =
	    stats->sum_score_in_decision / limmat->num_decisions;
	}
      else
	avg_score_in_decision = avg_assigned_in_decision = 0;

      fprintf (file,
	       LIMMAT_PREFIX
	       "average %.0f assigned variables per decision\n"
	       LIMMAT_PREFIX
	       "average %.1f variables rescored per decision\n"
	       LIMMAT_PREFIX
	       "average %.1f maximal score per decision\n"
	       LIMMAT_PREFIX
	       "maximal %d score\n",
	       avg_assigned_in_decision,
	       avg_rescored_variables,
	       avg_score_in_decision, limmat->max_score);

      fprintf (file,
	       LIMMAT_PREFIX
	       "\n"
	       LIMMAT_PREFIX
	       "%d clauses\n"
	       LIMMAT_PREFIX
	       "(%d orig, %.0f learned, %d max +%.0f -%.0f)\n",
	       limmat->num_clauses,
	       stats->original_clauses,
	       stats->learned_clauses,
	       stats->max_clauses,
	       limmat->added_clauses, stats->removed_clauses);

      fprintf (file,
	       LIMMAT_PREFIX
	       "\n"
	       LIMMAT_PREFIX
	       "%d literals\n"
	       LIMMAT_PREFIX
	       "(%d orig, %.0f learned, %d max +%.0f -%.0f)\n",
	       limmat->num_literals,
	       stats->original_literals,
	       stats->learned_literals,
	       stats->max_literals,
	       limmat->added_literals, stats->removed_literals);

      if (!stats->original_clauses)
	avg_original = 0;
      else
	avg_original =
	  stats->original_literals / (double) stats->original_clauses;

      if (!stats->learned_clauses)
	avg_learned = 0;
      else
	avg_learned =
	  stats->learned_literals / (double) stats->learned_clauses;

      if (!stats->max_clauses)
	avg_max = 0;
      else
	avg_max = stats->max_literals / (double) stats->max_clauses;

      if (!limmat->added_clauses)
	avg_added = 0;
      else
	avg_added = limmat->added_literals / (double) limmat->added_clauses;

      if (!stats->removed_clauses)
	avg_removed = 0;
      else
	avg_removed =
	  stats->removed_literals / (double) stats->removed_clauses;

      fprintf (file,
	       LIMMAT_PREFIX
	       "\n"
	       LIMMAT_PREFIX
	       "%.1f literals/clause\n"
	       LIMMAT_PREFIX
	       "(%.1f orig, %.1f learned, %.1f max +%.1f -%.1f)\n",
	       avg_num,
	       avg_original, avg_learned, avg_max, avg_added, avg_removed);
    }
  else
    {
      fprintf (file,
	       LIMMAT_PREFIX
	       "%d literals, %d clauses, %.1f literals/clause\n",
	       limmat->num_literals, limmat->num_clauses, avg_num);
    }

  if (stats)
    fprintf (file, LIMMAT_PREFIX "\n");

  fprintf (file,
	   LIMMAT_PREFIX
	   "%d variables with %d as maximal variable identifier\n",
	   limmat->num_variables, limmat->max_id);

  limmat->time += stop_timer (limmat->timer);
}

/*------------------------------------------------------------------------*/

void
strategy_Limmat (Limmat * limmat, FILE * file)
{
  start_timer (&limmat->timer);

  if (!file)
    file = stdout;

  fputs (LIMMAT_PREFIX
#ifdef NDEBUG
	 "-DEBUG "
#else
	 "+DEBUG "
#endif
#ifdef CHECK_CONSISTENCY
	 "+CHECK_CONSISTENCY "
#else
	 "-CHECK_CONSISTENCY "
#endif
#ifdef EXPENSIVE_CHECKS
	 "+EXPENSIVE_CHECKS"
#else
	 "-EXPENSIVE_CHECKS"
#endif
	 "\n", file);

  fputs (LIMMAT_PREFIX
#ifdef OPTIMIZE_DIRECTION
	 "+OPTIMIZE_DIRECTION "
#else
	 "-OPTIMIZE_DIRECTION "
#endif
#ifdef COMPACT_STACK
	 "+COMPACT_STACK"
#else
	 "-COMPACT_STACK"
#endif
	 "\n", file);

  fputs (LIMMAT_PREFIX
#ifdef EARLY_CONFLICT_DETECTION
	 "+EARLY_CONFLICT_DETECTION "
#else
	 "-EARLY_CONFLICT_DETECTION "
#endif
#ifdef SHORTEN_REASONS
	 "+SHORTEN_REASONS"
#else
	 "-SHORTEN_REASONS"
#endif
	 "\n", file);

  fputs (LIMMAT_PREFIX
#ifdef LOOK_AT_OTHER_WATCHED_LITERAL
	 "+LOOK_AT_OTHER_WATCHED_LITERAL"
#else
	 "-LOOK_AT_OTHER_WATCHED_LITERAL"
#endif
	 "\n", file);

  fprintf (file,
	   LIMMAT_PREFIX
	   "RESTART=%d "
	   "RESCORE=%d "
	   "RESCOREFACTOR=%f\n", 
	   limmat->restart.init, limmat->rescore.init,limmat->score_factor);

  limmat->time += stop_timer (limmat->timer);
}

/*------------------------------------------------------------------------*/

const char *
options_Limmat (void)
{
  return
#ifdef COMPACT_STACK
    "1"
#else
    "0"
#endif
#ifdef OPTIMIZE_DIRECTION
    "1"
#else
    "0"
#endif
#ifdef EARLY_CONFLICT_DETECTION
    "1"
#else
    "0"
#endif
#ifdef SHORTEN_REASONS
    "1"
#else
    "0"
#endif
#ifdef LOOK_AT_OTHER_WATCHED_LITERAL
    "1"
#else
    "0"
#endif
    ;
}

/*------------------------------------------------------------------------*/
/* Push a new frame on the control stack.  The stack is not modelled with an
 * arena, since frames are of fixed size and do not need alignment.
 */
static void
push_frame (Limmat * limmat, Variable * assignment)
{
  int old_bytes, new_bytes;
  Frame *frame;

  if (++limmat->decision_level >= limmat->control_size)
    {
      old_bytes = limmat->control_size * sizeof (Frame);
      limmat->control_size *= 2;
      new_bytes = limmat->control_size * sizeof (Frame);

      limmat->control =
	(Frame *) resize (limmat, limmat->control, new_bytes, old_bytes);
    }

  frame = limmat->control + limmat->decision_level;
  frame->decision = assignment;
  frame->decision_level = limmat->decision_level;
  frame->trail_level = count_Stack (&limmat->trail);
}

/*------------------------------------------------------------------------*/
/* Actually perform an assignment.  The 'assignment' field of the literal is
 * overwritten, the reason and the decision level are updated.  The
 * assignment is recorded on the trail for later backtracking purposes.
 */
static void
assign (Limmat * limmat, Assignment * assignment)
{
  Variable *stripped, *literal;
  int sign;

  if (limmat->stats)
    limmat->stats->assignments++;
  limmat->num_assigned++;
  assert (limmat->num_assigned >= 0);

#ifdef LOG_ASSIGNMENTS
  if (!limmat->dont_log)
    {
      fprintf (LOGFILE (limmat),
	       "ASSIGN\t%d@%d",
	       var2int (assignment->literal), assignment->decision_level);

      if (assignment->reason && assignment->reason->size > 1)
	{
	  fputs (" <= ", LOGFILE (limmat));

	  dump_clause_aux (LOGFILE (limmat), limmat,
			   assignment->reason, assignment->literal, 1, 1, 0,
			   0);
	}
      else
	fputc ('\n', LOGFILE (limmat));
    }
#endif

  literal = assignment->literal;
  sign = is_signed (literal);
  stripped = sign ? not (literal) : literal;

  stripped->assignment = (sign ? FALSE : TRUE);
  stripped->decision_level = assignment->decision_level;
  stripped->reason = assignment->reason;

  push (limmat, &limmat->trail, stripped);
}

/*------------------------------------------------------------------------*/

static void
print_time_prefix (Limmat * limmat)
{
  assert (limmat->log);

  limmat->time += restart_timer (&limmat->timer);
  fprintf (limmat->log, LIMMAT_PREFIX "   %7.2f  ", limmat->time);
}

/*------------------------------------------------------------------------*/

static void
one_line_stats (Limmat * limmat)
{
  FILE *log;
  Statistics *stats;

  stats = limmat->stats;
  log = limmat->log;

  if (log)
    {
      if (limmat->report.finished == 1)
	{
	  fprintf (log, LIMMAT_PREFIX "\n");
	  if (stats)
	    fprintf (log, LIMMAT_PREFIX "            ");
	  else
	    fprintf (log, LIMMAT_PREFIX "   seconds  ");
	  fprintf (log, "#decisions");

	  if (stats)
	    fprintf (log,
		     "          #propagations\n"
		     LIMMAT_PREFIX
		     "   seconds           #assignments             #visits");

	  fprintf (log, "\n" LIMMAT_PREFIX "\n");
	}

      print_time_prefix (limmat);

      fprintf (log, "%8.0f", limmat->num_decisions);

      if (stats)
	{
	  fprintf (log, " %9.0f %10.0f %11.0f",
		   stats->assignments, stats->propagations, stats->visits);
	}

      fputc ('\n', log);
      fflush (log);
    }
}

/*------------------------------------------------------------------------*/

static Variable *
next_decision (Limmat * limmat)
{
  Variable *res, *tmp, **order;
  Statistics *stats;
  int pos, score;

  assert (limmat->num_assigned < limmat->num_variables);

  stats = limmat->stats;
  pos = limmat->last_assigned_pos + 1;
  order = (Variable **) start_of_Stack (&limmat->order);

  /* Find a still unassigned variable at 'pos' or above.
   */
  res = 0;
  while (pos < count_Stack (&limmat->order) && !res)
    {
      tmp = order[pos];
      if (is_assigned (tmp))
	{
	  pos++;
	  if (stats)
	    stats->searched++;
	}
      else
	res = tmp;
    }

  assert (res);
  score = get_score (res);
  limmat->current_max_score = score;
  limmat->last_assigned_pos = pos - 1;

  return res;
}

/*------------------------------------------------------------------------*/

inline static void
swap (Limmat * limmat, Variable ** a, Variable ** b)
{
  Variable *tmp;

  if (limmat && limmat->stats)
    limmat->stats->swapped++;

  tmp = *a;
  *a = *b;
  *b = tmp;
}

/*------------------------------------------------------------------------*/

inline static void
cmpswap (Limmat * limmat, Variable ** a, Variable ** b)
{
  if (cmp (limmat, *a, *b) < 0)
    swap (limmat, a, b);
}

/*------------------------------------------------------------------------*/

inline static int
partition (Limmat * limmat, Variable ** a, int l, int r)
{
  Variable *v;
  int i, j;

  i = l - 1;
  j = r;
  v = a[r];

  for (;;)
    {
      while (cmp (limmat, a[++i], v) > 0)
	;

      while (cmp (limmat, v, a[--j]) > 0)
	if (j == l)
	  break;

      if (i >= j)
	break;

      swap (limmat, a + i, a + j);
    }

  swap (limmat, a + i, a + r);

  return i;
}

/*------------------------------------------------------------------------*/

inline static void
insertion (Limmat * limmat, Variable ** a, int l, int r)
{
  Variable *v;
  int i, j;

  for (i = r; i > l; i--)
    cmpswap (limmat, a + i - 1, a + i);

  for (i = l + 2; i <= r; i++)
    {
      j = i;
      v = a[i];
      while (cmp (limmat, v, a[j - 1]) > 0)
	{
	  a[j] = a[j - 1];
	  j--;
	}
      a[j] = v;
    }
}

/*------------------------------------------------------------------------*/

#define ISORT_LIMIT 10

/*------------------------------------------------------------------------*/

inline static void
quicksort (Limmat * limmat, Variable ** a, int l, int r)
{
  int i, m, l_to_push, r_to_push;
  Stack *stack;

#if 1
  static int isort_limit = -1;
  if (isort_limit < 0)
    {
      if (getenv ("ISORT"))
	isort_limit = atoi (getenv ("ISORT"));
      if (isort_limit < 0)
	isort_limit = ISORT_LIMIT;
    }
#undef ISORT_LIMIT
#define ISORT_LIMIT isort_limit
#endif

  assert (ISORT_LIMIT >= 3);
  if (r - l <= ISORT_LIMIT)
    return;

  stack = &limmat->stack;
  reset_Stack (limmat, stack, 0);

  for (;;)
    {
#if 0
      fprintf (stdout, "%d\n", r - l);
#endif
      m = (l + r) / 2;
      swap (limmat, a + m, a + r - 1);
      cmpswap (limmat, a + l, a + r - 1);
      cmpswap (limmat, a + l, a + r);
      cmpswap (limmat, a + r - 1, a + r);
      i = partition (limmat, a, l + 1, r - 1);

      if (i - l < r - i)
	{
	  l_to_push = i + 1;
	  r_to_push = r;
	  r = i - 1;
	}
      else
	{
	  l_to_push = l;
	  r_to_push = i - 1;
	  l = i + 1;
	}

      if (r - l > ISORT_LIMIT)
	{
	  assert (r_to_push - l_to_push > ISORT_LIMIT);
	  push (limmat, stack, (void *) (PTR_SIZED_WORD) l_to_push);
	  push (limmat, stack, (void *) (PTR_SIZED_WORD) r_to_push);
	}
      else if (r_to_push - l_to_push > ISORT_LIMIT)
	{
	  l = l_to_push;
	  r = r_to_push;
	}
      else if (count_Stack (stack))
	{
	  r = (int) (PTR_SIZED_WORD) pop (stack);
	  l = (int) (PTR_SIZED_WORD) pop (stack);
	}
      else
	break;
    }
}

/*------------------------------------------------------------------------*/
/* This is the hyper sorting mechanism from the Sedgewick book.
 */
static void
hypersort (Limmat * limmat, Variable ** a, int l, int r)
{
  quicksort (limmat, a, l, r);
  insertion (limmat, a, l, r);
}

/*------------------------------------------------------------------------*/

static void
sort_order_aux (Limmat * limmat)
{
  Variable **base;
  int end;

  base = start_of_Stack (&limmat->order);
  end = limmat->first_zero_pos - 1;
  hypersort (limmat, base, 0, end);
}

/*------------------------------------------------------------------------*/

static void
adjust_search_interval (Limmat * limmat)
{
  int i, end, last_non_zero_pos, first_unassigned_pos;
  Variable **variables, *v;

  variables = start_of_Stack (&limmat->order);
  end = limmat->first_zero_pos - 1;
  last_non_zero_pos = -1;
  first_unassigned_pos = end + 1;

  for (i = 0; i <= end; i++)
    {
      v = variables[i];
      if (first_unassigned_pos > end && !is_assigned (v))
	first_unassigned_pos = i;
      if (get_score (v))
	last_non_zero_pos = i;
      set_pos (v, i);
    }

  assert (last_non_zero_pos < limmat->first_zero_pos);
  limmat->first_zero_pos = last_non_zero_pos + 1;
  limmat->last_assigned_pos = first_unassigned_pos - 1;
}

/*------------------------------------------------------------------------*/
#if !defined(NDEBUG) || defined(LIMMAT_WHITE)
/*------------------------------------------------------------------------*/

static int
is_sorted (Limmat * limmat)
{
  Variable **order;
  int i, end, res;

  order = start_of_Stack (&limmat->order);
  end = limmat->first_zero_pos - 1;

  for (i = 0, res = 1; res && i < end; i++)
    res = (cmp (0, order[i], order[i + 1]) > 0);

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Sort the array between from the start until 'limmat->first_zero_pos' (the
 * latter excluded).  We would like to start the sorting at
 * 'limmat->last_assigned_pos+1' but we have to take into account that
 * backtracking may actually unassign some variables with large positive
 * score.  Therefore those variables have to be sorted as well.
 */
static void
sort_order (Limmat * limmat)
{
  if (count_Stack (&limmat->order))
    {
      sort_order_aux (limmat);
      adjust_search_interval (limmat);
      assert (is_sorted (limmat));
      invariant (limmat);
    }
}

/*------------------------------------------------------------------------*/
/* Initialize the score of a literal that is used for sorting by the number
 * of its occurences in clauses.  Also adjust 'first_zero_pos'.
 */
static void
init_score (Limmat * limmat)
{
  int ref, i, last_non_zero_pos, pos;
  Variable *v, *end;

  last_non_zero_pos = -1;
  forall_variables (limmat, v, end)
  {
    for (i = 0; i < 2; i++)
      {
	ref = v->ref[i];
	assert (ref >= 0);
	v->saved[i] = ref;
	v->score[i] = ref;
	if (ref > limmat->current_max_score)
	  {
	    limmat->current_max_score = ref;
	    if (ref > limmat->max_score)
	      limmat->max_score = ref;
	  }

	pos = v->pos[i];
	if (ref > 0 && pos > last_non_zero_pos)
	  last_non_zero_pos = pos;
      }
  }
  limmat->first_zero_pos = last_non_zero_pos + 1;
}

/*------------------------------------------------------------------------*/

static void
init_order (Limmat * limmat)
{
  init_score (limmat);
  sort_order (limmat);
}

/*------------------------------------------------------------------------*/
/* First divide the score of all variables by a certain amount and add the
 * delta of new references since the last rescore.  Then sort the variables
 * with respect to the new calculated score.
 */
static void
rescore (Limmat * limmat)
{
  int sign, ref, score, delta_ref, new_score, pos, end;
  int max_new_score, last_non_zero_pos;
  Variable *v, **variables;

#ifdef LOG_RESCORE
  if (!limmat->dont_log)
    fprintf (LOGFILE (limmat), "RESCORE\t%.0f\n", limmat->rescore.finished);
#endif

  last_non_zero_pos = 0;
  end = limmat->first_zero_pos - 1;
  variables = start_of_Stack (&limmat->order);
  assert (variables);

  max_new_score = -1;

  for (pos = 0; pos <= end; pos++)
    {
      v = variables[pos];

      assert (get_pos (v) == pos);
      sign = is_signed (v);
      if (sign)
	v = not (v);

      ref = v->ref[sign];
      delta_ref = ref - v->saved[sign];
      v->saved[sign] = ref;
      score = v->score[sign];

      if (score || delta_ref)
	{
	  assert (delta_ref >= 0);
	  assert (score >= 0);

	  new_score = score * (double) limmat->score_factor;

	  if (delta_ref > 0)
	    {
	      if (limmat->stats)
		limmat->stats->rescored_variables++;
	      new_score += delta_ref;
	    }

	  if (new_score)
	    {
	      if (pos > last_non_zero_pos)
		last_non_zero_pos = pos;
	    }

	  if (new_score > max_new_score)
	    max_new_score = new_score;

	  assert (new_score >= 0);
	  v->score[sign] = new_score;
	}
    }

  limmat->current_max_score = max_new_score;
  if (max_new_score > limmat->max_score)
    limmat->max_score = max_new_score;

  limmat->first_zero_pos = last_non_zero_pos + 1;
  sort_order (limmat);
}

/*------------------------------------------------------------------------*/
/* Derive assignments from all unit clauses.
 */
static void
units2mark (Limmat * limmat)
{
  Clause **p, **eoc, *clause;
  Assignment assignment;

  assignment.reason = 0;
  assignment.decision_level = -1;

  forall_Stack (&limmat->units, Clause *, p, eoc)
  {
    clause = *p;
    assert (clause->size == 1);
    assignment.literal = lit2var (clause2literals (limmat, clause)[0]);
    push_assignment (limmat, &assignment);
  }
}

/*------------------------------------------------------------------------*/

static void
restart (Limmat * limmat)
{
  assert (limmat->decision_level >= 0);

#ifdef LOG_RESTART
  if (!limmat->dont_log)
    fprintf (LOGFILE (limmat), "RESTART\t%.0f\n", limmat->restart.finished);
#endif

  untrail (limmat, -1);
  limmat->decision_level = -1;
  init_order (limmat);
  units2mark (limmat);
  invariant (limmat);
}

/*------------------------------------------------------------------------*/

static int
its_time_to_rescore (Limmat * limmat)
{
  int res;

  dec_Counter (&limmat->rescore);
  res = done_Counter (&limmat->rescore);

  return res;
}

/*------------------------------------------------------------------------*/
/* Search for a still unassigned decision literal and push it on the
 * assignment queue.
 */
static void
decide (Limmat * limmat)
{
  Assignment assignment;
  Variable *decision;
  Statistics *stats;
  int score;

  if (its_time_to_rescore (limmat))
    rescore (limmat);

  decision = next_decision (limmat);
  assert (decision);

  stats = limmat->stats;

  if (stats)
    {
      stats->sum_assigned_in_decision += limmat->num_assigned;
      score = get_score (decision);
      assert (score == limmat->current_max_score);
      stats->sum_score_in_decision += score;
    }

  limmat->num_decisions++;
  push_frame (limmat, decision);

  assignment.decision_level = limmat->decision_level;
  assignment.literal = decision;
  assignment.reason = 0;
  push_assignment (limmat, &assignment);

  if (limmat->log)
    {
      dec_Counter (&limmat->report);
      if (done_Counter (&limmat->report))
	one_line_stats (limmat);
    }

#ifdef LOG_DECISION
  if (!limmat->dont_log)
    {
      fprintf (LOGFILE (limmat),
	       "DECIDE\t%d@%d\n", var2int (decision), limmat->decision_level);
    }
#endif
}

/*------------------------------------------------------------------------*/
#ifdef EXPENSIVE_CHECKS
/*------------------------------------------------------------------------*/

static void
check_clause_is_implied (Limmat * limmat, Clause * new_clause)
{
#if 0
#warning "check_clause_is_implied disabled"
#else
  Clause *clause, **p, **eoc;
  int i, id, n, res;
  Variable *v;
  Limmat *tmp;

  static int sat (Limmat *, int);

  if (!check_it_now (limmat))
    return;

  tmp = new_Limmat (0);
#ifdef LOG_SOMETHING
  tmp->dont_log = 1;		/* avoid logging in call to 'sat(tmp,...)' below */
#endif
  tmp->inner_mode = 1;
  init_Counter (&tmp->restart, 0, 0, 0);
  init_Counter (&tmp->rescore, 0, 0, 0);

  /* Avoid moving 'tmp->variables.data_start'.
   */
  (void) find (tmp, limmat->max_id);

  forall_clauses (limmat, p, eoc)
  {
    clause = *p;
    n = clause->size;

    if (clause == new_clause)
      {
	for (i = 0; i < n; i++)
	  {
	    id =
	      var2int (not (lit2var (clause2literals (limmat, clause)[i])));
	    v = find (tmp, id);
	    add_clause (tmp, &v, 1, 0);
	  }
      }
    else
      {
	assert (!count_Stack (&tmp->stack));

	for (i = 0; i < n; i++)
	  {
	    id = var2int (lit2var (clause2literals (limmat, clause)[i]));
	    v = find (tmp, id);
	    push (tmp, &tmp->stack, v);
	  }

	add_clause (tmp, (Variable **) start_of_Stack (&tmp->stack), n, 0);
	reset_Stack (limmat, &tmp->stack, 0);
      }
  }

  res = sat (tmp, 0);		/* unit resolution should show unsatisfiability */
  assert (!res);
  delete_Limmat (tmp);
#endif
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
/* Add all literals on the 'literals' stack as a clause to the clause data
 * base and generate a conflict driven assignment.
 */
static void
add_literals (Limmat * limmat, Assignment * assignment)
{
  Variable **literals;
  Clause *clause;
  int count;

  count = count_Stack (&limmat->clause);
  assert (count);

  if (limmat->stats)
    limmat->stats->learned_clauses++;
  if (limmat->stats)
    limmat->stats->learned_literals += count;

  literals = (Variable **) start_of_Stack (&limmat->clause);
  clause = add_clause (limmat, literals, count, assignment);
  clause->learned = 1;
  reset_Stack (limmat, &limmat->clause, 0);

#ifdef LOG_LEARNED
  if (!limmat->dont_log)
    {
      fprintf (LOGFILE (limmat), "LEARN\t");
      dump_clause_aux (LOGFILE (limmat), limmat, clause, 0, 0, 1, 0, 0);
    }
#endif

#ifdef EXPENSIVE_CHECKS
  check_clause_is_implied (limmat, clause);
#endif
}

/*------------------------------------------------------------------------*/
#ifdef EXPENSIVE_CHECKS
/*------------------------------------------------------------------------*/

static void
check_all_variables_unmarked (Limmat * limmat)
{
  Variable *v, *eov;

  forall_variables (limmat, v, eov) assert (!v->mark);
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

inline static void
push_literal (Limmat * limmat, Variable * v)
{
  Variable *new_literal;

  assert (!is_signed (v));
  assert (is_assigned (v));

  new_literal = ((v->assignment == TRUE) ? not (v) : v);
  push (limmat, &limmat->clause, new_literal);
}

/*------------------------------------------------------------------------*/
/* One step in the breadth first backward search through the dependency
 * graph of the assignments on the current decision level consists of
 * pushing all unvisited variables onto to the 'visited' stack and updating
 * the number of open paths from the conflict backward to this variable.
 */
inline static void
expand_open_paths (Limmat * limmat, Clause * clause, int *paths)
{
  Variable *v, **p, **eol;

  forall_literals (limmat, clause, p, eol)
  {
    v = strip (lit2var (*p));

    if (v->decision_level >= 0 && !v->mark)
      {
	v->mark = 1;

	/* Restrict the search to variables on the current decision level.
	 * The rest of the dependency graph is masked out.
	 */
	if (v->decision_level == limmat->decision_level)
	  {
	    /* This is the first time we visit this variable. So we have
	     * to increase the number of paths back from the conflict by
	     * one.
	     */
	    *paths += 1;
	  }
	else
	  {
	    /* All variables with lower decision level become part of the
	     * conflict clause.
	     */
	    assert (v->decision_level < limmat->decision_level);
	    push_literal (limmat, v);
	  }
      }
  }
}

/*------------------------------------------------------------------------*/
/* Reset all the marked bits of the literals in the clause in case they have
 * a smaller decision level as the current decision level.  These variables
 * should have been marked, when they are pushed on the literals stack for
 * generating the conflict clause.
 */
static void
unmark_clause_after_conflict_generation (Limmat * limmat, Clause * clause)
{
  Variable **p, **eol, *v;

  forall_literals (limmat, clause, p, eol)
  {
    v = strip (lit2var (*p));
    assert (v->decision_level <= limmat->decision_level);
    if (v->decision_level >= 0)
      if (v->decision_level < limmat->decision_level)
	v->mark = 0;
  }
}

/*------------------------------------------------------------------------*/
/* Reset the 'mark' bits of all variables traversed and marked in
 * 'expand_open_paths'.  Make sure to unmark the literals in the 'reason'
 * for a variable assignment as well.
 */
static void
unmark_after_conflict_generation (Limmat * limmat, Variable * v)
{
  Clause *reason;

  assert (v->decision_level == limmat->decision_level);

  if (v->mark)
    {
      v->mark = 0;
      reason = v->reason;
      if (reason)
	unmark_clause_after_conflict_generation (limmat, reason);
    }
}

/*------------------------------------------------------------------------*/
/* A unique implication point (uip) is an assigned variable in the current
 * decision level that occurs on all paths back from the conflict ignoring
 * edges leading to variables with lower decision level.  The decision
 * variable of the current decision level acts as a sentinel to this search.
 * The reverse order of the assigned variables on the trail is a topological
 * order of the dependency graph.  Thus we do not need a general graph
 * algorithm to decompose a graph in its biconnected components.
 */
static void
generate_conflict_clause (Limmat * limmat, Clause * conflict)
{
  Variable **assigned, **last_assigned, **first_assigned, **p, *v;
  Clause *reason;
  Frame *frame;
  int paths;

  assert (!count_Stack (&limmat->clause));
  assert (limmat->decision_level >= 0);

  frame = limmat->control + limmat->decision_level;
  assigned = (Variable **) start_of_Stack (&limmat->trail);
  first_assigned = assigned + frame->trail_level;
  last_assigned = assigned + count_Stack (&limmat->trail) - 1;

#ifdef NDEBUG
  if (first_assigned > assigned)
    assert (first_assigned[-1]->decision_level < limmat->decision_level);
#endif

  /* Traverse all assigned variables in the current decision level starting
   * with the most recently assigned variable.  Stop as soon as a unique
   * implication point is found.  During the traversal record the already
   * visited variables by setting their 'mark' bit.  Additionally count the
   * number of open paths to detect unique implication points.  In
   * particular a unique implication point is found if all paths converged,
   * thus the number of still open branches is zero.
   */
  v = 0;
  paths = 0;
  expand_open_paths (limmat, conflict, &paths);

  for (p = last_assigned; paths; p--)
    {
      assert (p >= first_assigned);

      v = strip (*p);
      assert (v->decision_level == limmat->decision_level);

      if (v->mark)
	{
	  paths--;
	  reason = v->reason;
	  if (!reason)
	    assert (!paths);

	  if (paths)
	    expand_open_paths (limmat, reason, &paths);
	  else
	    push_literal (limmat, v);
	}
    }

  assert (v);

  /* Reset the mark bit of all traversed variables.
   */
  while (++p <= last_assigned)
    unmark_after_conflict_generation (limmat, *p);

  /* And do not forget lower decision level variables referenced in the
   * conflict clause only.
   */
  unmark_clause_after_conflict_generation (limmat, conflict);

#ifdef EXPENSIVE_CHECKS
  check_all_variables_unmarked (limmat);
#endif

  if (v->reason)
    {
      if (limmat->stats)
	limmat->stats->uips++;

#ifdef LOG_UIP
      if (!limmat->dont_log)
	{
	  int sign;

	  assert (is_assigned (v));
	  sign = (v->assignment == FALSE) ? -1 : 1;
	  fprintf (LOGFILE (limmat),
		   "UIP\t%d@%d\n", sign * var2int (v), v->decision_level);
	}
#endif
    }
}

/*------------------------------------------------------------------------*/
/* Analyze the current conflict and deduce a conflict driven assignment.
 * In case the empty clause is implied use 'FALSE' as 'literal' in the
 * assignment.
 */
static void
analyze (Limmat * limmat, Clause * conflict, Assignment * assignment)
{
  assignment->literal = FALSE;

  if (limmat->decision_level >= 0)
    {
      generate_conflict_clause (limmat, conflict);

      if (count_Stack (&limmat->clause) > 0)
	add_literals (limmat, assignment);
    }
}

/*------------------------------------------------------------------------*/

static int
backtrack (Limmat * limmat, Clause * conflict)
{
  Assignment assignment;
  int res, delta;

  if (limmat->decision_level < 0)
    return 0;

  assert (conflict);

  reset_assignments (limmat);
  analyze (limmat, conflict, &assignment);
  res = (assignment.literal != FALSE);
  untrail (limmat, assignment.decision_level + 1);
  delta = limmat->decision_level - assignment.decision_level;
  assert (delta >= 0);
  limmat->decision_level = assignment.decision_level;

  if (limmat->stats && delta > 1)
    {
      limmat->stats->backjumps += delta - 1;

#ifdef LOG_BACKJUMP
      if (!limmat->dont_log)
	fprintf (LOGFILE (limmat), "JUMP\t%d\n", delta - 1);
#endif
    }

  if (res)
    {
      push_assignment (limmat, &assignment);

#ifdef LOG_BACKTRACK
      if (!limmat->dont_log)
	fprintf (LOGFILE (limmat),
		 "BACK\t%d@%d\n",
		 var2int (assignment.literal), assignment.decision_level);
#endif
    }
  else
    {
#ifdef LOG_BACKTRACK
      if (!limmat->dont_log)
	fprintf (LOGFILE (limmat), "BACK\ttop\n");
#endif
    }

  invariant (limmat);

  return res;
}

/*------------------------------------------------------------------------*/
#if !defined(NDEBUG) && defined(EXPENSIVE_CHECKS)
/*------------------------------------------------------------------------*/

static int
max_decision_level_clause (Limmat * limmat, Clause * clause)
{
  Variable *literal;
  int i, res;

  for (res = -1, i = 0; i < clause->size; i++)
    {
      literal = strip (lit2var (clause2literals (limmat, clause)[i]));
      if (literal->decision_level > res)
	res = literal->decision_level;
    }

  return res;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

inline static int
next_pos (int prev,
	  int *direction_ptr, int start, int num_literals, int *turned_ptr)
{
  int res, direction;

  direction = *direction_ptr;
  res = prev + direction;

  if (res < 0 || res >= num_literals)
    {
      direction *= -1;
      *direction_ptr = direction;
      res = start + direction;

      if (res < 0 || res >= num_literals)
	*turned_ptr = 2;
      else
	*turned_ptr += 1;

    }

  return res;
}

/*------------------------------------------------------------------------*/
/* This is the innermost propagation operation.  It assumes that the watched
 * occurence of the literal is assigned to false and the watched literals in
 * the clause have to be adjusted accordingly.  In this process we
 * also detect unit clauses and conflicts.
 */
inline static Clause *
propagate_clause (Limmat * limmat,
		  Occurence signed_clause, int decision_level, int *remove)
{
  int sign, start, head, tail, i, num_literals, turned, direction;
  Variable *tmp, **literals, *dereferenced_tail;
  Variable *literal, *tail_literal;
  Clause *conflict, *clause;
  Assignment new_assignment;

  if (limmat->stats)
    limmat->stats->propagations++;

#ifdef LOG_PROPS
  fprintf (LOGFILE (limmat), "PROPS\t%.0f\t", limmat->stats->propagations);
  dump_clause (limmat, (Clause *) strip (signed_clause));
#endif

  conflict = 0;
  *remove = 0;

  sign = is_signed (signed_clause);
  clause = sign ? not (signed_clause) : signed_clause;

  /* The tail 'watched' literal can be found by flipping the sign.
   */
  tail = clause->watched[!sign];

  if (tail < 0)
    {
      assert (clause->size == 1);
      conflict = clause;
    }
  else
    {
      if (limmat->stats)
	limmat->stats->visits++;

      literals = clause2literals (limmat, clause);
#ifndef LOOK_AT_OTHER_WATCHED_LITERAL
      tail_literal = 0;
      dereferenced_tail = FALSE;
#else
      tail_literal = lit2var (literals[tail]);
      dereferenced_tail = deref (tail_literal);
      if (dereferenced_tail == FALSE)
	conflict = clause;
      else if (dereferenced_tail != TRUE)
#endif
	{
	  head = clause->watched[sign];
	  num_literals = clause->size;

	  assert (num_literals >= 2);
	  assert (head >= 0);
	  assert (deref (lit2var (literals[head])) == FALSE);

	  /* Now try to find an unassigned literal among the literals of the
	   * clause.  Stop as soon a literal assigned to 'TRUE' is found or
	   * we wrap around and reach 'start' again.  Prepare a new
	   * assignment, in case a unit clause has been found.
	   */
#ifdef OPTIMIZE_DIRECTION
	  direction = clause->direction;
#else
	  direction = 1;
#endif
	  turned = 0;
	  start = head;
	  i = start;

	  tmp = FALSE;

	  while (tmp == FALSE && turned < 2)
	    {
#ifdef LOOK_AT_OTHER_WATCHED_LITERAL
	      /* Just skip the 'tail', since we already checked it.  Also
	       * skip the 'head' since it should always be assigned to
	       * FALSE.
	       */
	      if (i != tail)
#endif
		if (i != head)
		  {
		    if (limmat->stats)
		      limmat->stats->visits++;

#ifndef LOOK_AT_OTHER_WATCHED_LITERAL
		    if (i == tail)
		      {
			tail_literal = lit2var (literals[i]);
			dereferenced_tail = deref (tail_literal);
			if (dereferenced_tail == TRUE)
			  break;
		      }
		    else
#endif
		      {
			literal = lit2var (literals[i]);
			tmp = deref (literal);
		      }
		  }

	      if (tmp == FALSE)
		i = next_pos (i, &direction, start, num_literals, &turned);
	    }

#ifndef LOOK_AT_OTHER_WATCHED_LITERAL
	  if (tmp == FALSE && dereferenced_tail == FALSE)
	    conflict = clause;
	  else if (dereferenced_tail == TRUE)
	    {
	      /* nothing to be done */
	    }
	  else
#endif

	  if (turned >= 2)
	    {
	      /* We wrapped around without finding a non false assigned
	       * literal, except the tail literal.  As a consequence assign
	       * the tail literal to true with the maximum level encountered
	       * while traversing the clause.  Keep this occurence of the
	       * literal as watched.
	       */
	      assert (tail_literal);
	      new_assignment.literal = tail_literal;
	      new_assignment.decision_level = decision_level;
	      new_assignment.reason = clause;
	      push_assignment (limmat, &new_assignment);
	    }
	  else
	    {
	      /* We found a non false literal which will become the new
	       * watched literal at 'sign'.  This should be the standard
	       * case and we have to notify the calling function that this
	       * occurence can be removed.
	       */
	      assert (i != tail);
	      partial_disconnect (limmat, signed_clause, start);
	      connect (limmat, signed_clause, i);
	      *remove = 1;
#ifdef OPTIMIZE_DIRECTION
	      clause->direction = direction;
#endif
	    }
	}
    }

  invariant_clause (limmat, clause);

  return conflict;
}

/*------------------------------------------------------------------------*/

inline static Clause *
propagate_assignment (Limmat * limmat, Assignment * assignment)
{
  Occurence *occurrences;
  int i, count, remove;
  Clause *conflict;
  Stack *stack;

#ifdef EXPENSIVE_CHECKS
  if (assignment->reason)
    assert (max_decision_level_clause (limmat, assignment->reason)
	    <= assignment->decision_level);
#endif

  /* First actually assign the literal.
   */
  assign (limmat, assignment);

  /* Traverse all occurrences that contain a watched negated literal of the
   * assignment, as long no conflict has been found.
   */
  stack = var2stack (not (assignment->literal));
  occurrences = start_of_Stack (stack);
  count = count_Stack (stack);
  conflict = 0;
  i = 0;

  while (!conflict && i < count)
    {
      conflict =
	propagate_clause (limmat, occurrences[i], assignment->decision_level,
			  &remove);

      /* To remove the occurence from the stack we just overwrite it with
       * the last entry in the stack and pop the last Element of the stack.
       * We do not want to reset the stack until the end, because we have to
       * assume that stacks may shrink if their size is decreased.
       */
      if (!conflict)
	{
	  if (remove)
	    {
	      if (--count > 0)
		occurrences[i] = occurrences[count];
	    }
	  else
	    i++;
	}
    }

  reset_Stack (limmat, stack, count);

  return conflict;
}

/*------------------------------------------------------------------------*/
/* Propagate assignments until the assignments stack is empty or a conflict 
 * has been generated.
 */
static Clause *
propagate (Limmat * limmat)
{
  Variable *literal, *tmp;
  Assignment assignment;
  Clause *conflict;

  conflict = 0;

  while (!conflict && count_assignments (limmat) > 0)
    {
      /* Pop next assignment from mark stack.  Each assignment is made of
       * the literal that will be assigned to true, the level at which this
       * assignment has been generated and finally the clause that triggered
       * the assignment (0 if decision or toplevel unit).
       */
      pop_assignment (limmat, &assignment);

      /* We dereference the literal.  It should not be assigned to 'FALSE'
       * since that would have triggered an earlier conflict.
       */
      literal = assignment.literal;
      tmp = deref (literal);
#ifdef SHORTEN_REASONS
      assert (tmp != TRUE);
#else
      if (tmp == TRUE)
	continue;
#endif
      if (tmp == FALSE)
	conflict = assignment.reason;
      else
	conflict = propagate_assignment (limmat, &assignment);
    }

#ifdef LOG_CONFLICT
  if (conflict && !limmat->dont_log)
    {
      fprintf (LOGFILE (limmat), "BOOM\t");
      dump_clause_aux (LOGFILE (limmat), limmat, conflict, 0, 1, 1, 0, 0);
    }
#endif

  if (conflict)
    limmat->num_conflicts++;

  return conflict;
}

/*------------------------------------------------------------------------*/

static int
all_clauses_satisfied (Limmat * limmat)
{
  Variable **q, **eol;
  Clause **p, **eoc;
  int res, found;

  res = !limmat->contains_empty_clause;

  forall_clauses (limmat, p, eoc)
  {
    found = 0;

    forall_literals (limmat, *p, q, eol)
      if ((found = (deref (lit2var (*q)) == TRUE)))
      break;

    if (!(res = found))
      break;
  }

  return res;
}

/*------------------------------------------------------------------------*/

static int
its_time_to_restart (Limmat * limmat)
{
  int res;

  if (limmat->decision_level >= 0)
    {
      dec_Counter (&limmat->restart);
      res = done_Counter (&limmat->restart);
    }
  else
    res = 0;

  return res;
}

/*------------------------------------------------------------------------*/

static int
sat (Limmat * limmat, int max_decisions)
{
  int res, num_decisions;
  Clause *conflict;

  if (limmat->contains_empty_clause)
    res = 0;
  else
    {
      init_order (limmat);
      units2mark (limmat);

      num_decisions = 0;
      res = -1;

      while (res < 0)
	{
	  if (count_assignments (limmat))
	    {
	      conflict = propagate (limmat);
	      if (conflict && !backtrack (limmat, conflict))
		res = 0;
	    }
	  else if (limmat->num_assigned < limmat->num_variables)
	    {
	      if (max_decisions >= 0)
		{
		  if (num_decisions >= max_decisions)
		    break;

		  num_decisions++;
		}

	      if (its_time_to_restart (limmat))
		restart (limmat);
	      else
		decide (limmat);
	    }
	  else
	    {
#ifdef LOG_DECISION
	      if (!limmat->dont_log)
		fprintf (LOGFILE (limmat), "NO MORE VARIABLES LEFT\n");
#endif

	      res = 1;
	    }
	}
    }

  if (limmat->log && limmat->report.finished > 0)
    fprintf (limmat->log, LIMMAT_PREFIX "\n");

  if (res == 1 && !all_clauses_satisfied (limmat))
    {
      fprintf (stderr, "*** limmat internal error: inconsistent result\n");
      fflush (stderr);
      abort ();
    }

  return res;
}

/*------------------------------------------------------------------------*/

int
sat_Limmat (Limmat * limmat, int max_decisions)
{
  int res;

  start_timer (&limmat->timer);
  res = sat (limmat, max_decisions);
  limmat->time += stop_timer (limmat->timer);

  return res;
}

/*------------------------------------------------------------------------*/

const int *
assignment_Limmat (Limmat * limmat)
{
  Variable *tmp, *v, *end;
  int *res, literal;

  start_timer (&limmat->timer);
  if (all_clauses_satisfied (limmat))
    {
      reset_Stack (limmat, &limmat->clause, 0);

      forall_variables (limmat, v, end)
      {
	tmp = deref (v);
	assert (tmp == FALSE || tmp == TRUE);
	literal = v->id;
	if (tmp == FALSE)
	  literal = -literal;
	push (limmat, &limmat->clause, (void *) (PTR_SIZED_WORD) literal);
      }

      push (limmat, &limmat->clause, (void *) (int) 0);
      res = intify_Stack (&limmat->clause);
    }
  else
    res = 0;
  limmat->time += stop_timer (limmat->timer);

  return res;
}

/*------------------------------------------------------------------------*/

#define BUFFER_SIZE 100

/*------------------------------------------------------------------------*/

static int
intlen (int d)
{
  int sign, res;

  if (d)
    {
      if ((sign = (d < 0)))
	d = -d;

      if (d < 10)
	res = 1;
      else if (d < 100)
	res = 2;
      else if (d < 1000)
	res = 3;
      else if (d < 10000)
	res = 4;
      else if (d < 100000)
	res = 5;
      else if (d < 1000000)
	res = 6;
      else if (d < 10000000)
	res = 7;
      else if (d < 100000000)
	res = 8;
      else if (d < 1000000000)
	res = 9;
      else
	res = 10;		/* fine for 32 bit machines */

      if (sign)
	res++;
    }
  else
    res = 1;

  return res;
}

/*------------------------------------------------------------------------*/

inline static int
insert_buffer (int val, int next, int pos, char *buffer)
{
  int l;

  if (pos)
    assert (!buffer[pos]);

  if (pos)
    buffer[pos++] = ' ';
  sprintf (buffer + pos, "%d", val);
  assert (intlen (val) == strlen (buffer + pos));
  pos += intlen (val);
  l = intlen (next);

  if (pos + l > 76)
    {
      assert (pos + 1 < BUFFER_SIZE);
      buffer[pos] = 0;
      pos = 0;
    }

  return pos;
}

/*------------------------------------------------------------------------*/

void
print_assignment_Limmat (const int *assignment, FILE * file)
{
  char buffer[BUFFER_SIZE];
  const int *p;
  int pos;

  pos = 0;
  buffer[pos] = 0;

  for (p = assignment; *p; p++)
    {
      pos = insert_buffer (p[0], p[1], pos, buffer);
      if (!pos)
	{
#ifdef SAT2002FMT
	  fputs ("v ", file);
#endif
	  fprintf (file, "%s\n", buffer);
	}
    }

  if (pos)
    {
#ifdef SAT2002FMT
      fputs ("v ", file);
#endif
      fprintf (file, "%s\n", buffer);
    }
}

/*------------------------------------------------------------------------*/
#ifdef LIMMAT_WHITE
/*------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

/*------------------------------------------------------------------------*/

typedef struct Suite Suite;

struct Suite
{
  int argc, ok, failed, backspaces;
  char **argv;
};

/*------------------------------------------------------------------------*/
/* Compare two files for differences.  The result is '1' iff no differences
 * were encountered and both files could be accessed.  If one file could not
 * be openend, then '0' is returned.
 */
static int
cmp_files (char *name0, char *name1)
{
  int res, ch;
  FILE *f[2];

  f[0] = fopen (name0, "r");
  if (f[0])
    {
      f[1] = fopen (name1, "r");
      if (f[1])
	{
	  do
	    {
	      ch = fgetc (f[0]);
	      res = (fgetc (f[1]) == ch);
	    }
	  while (res && ch != EOF);
	  fclose (f[1]);
	}
      else
	res = 0;
      fclose (f[0]);
    }
  else
    res = 0;

  return res;
}

/*------------------------------------------------------------------------*/

static int
mem0 (void)
{
  Limmat limmat;
  void *ptr;
  int res;

  limmat.bytes = 0;
  limmat.max_bytes = 0;
  ptr = new (&limmat, 4711);
  res = (limmat.bytes >= 4711);
  delete (&limmat, ptr, 4711);
  if (res)
    res = !limmat.bytes;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_MEM1 1000
#define M_MEM1 1000

static int
mem1 (void)
{
  void *ptr[N_MEM1];
  Limmat limmat;
  int res, i;

  limmat.bytes = 0;
  limmat.max_bytes = 0;
  for (i = 0; i < N_MEM1; i++)
    ptr[i] = new (&limmat, M_MEM1);
  res = (limmat.bytes >= M_MEM1 * N_MEM1);
  for (i = 0; i < N_MEM1; i++)
    delete (&limmat, ptr[i], M_MEM1);
  if (res)
    res = !limmat.bytes;

  return res;
}

/*------------------------------------------------------------------------*/

static int
mem2 (void)
{
  void *ptr[5], *tmp;
  Limmat limmat;

  limmat.bytes = 0;
  limmat.max_bytes = 0;
  ptr[0] = new (&limmat, 17);
  ptr[1] = new (&limmat, 18);
  ptr[2] = new (&limmat, 19);
  ptr[3] = new (&limmat, 20);
  ptr[4] = new (&limmat, 21);
  tmp = malloc (17);
  ptr[4] = resize (&limmat, ptr[4], 222, 21);
  free (tmp);
  ptr[0] = resize (&limmat, ptr[0], 5, 17);
  ptr[2] = resize (&limmat, ptr[2], 18, 19);
  ptr[3] = resize (&limmat, ptr[3], 20, 20);
  delete (&limmat, ptr[1], 18);
  delete (&limmat, ptr[3], 20);
  delete (&limmat, ptr[4], 222);
  delete (&limmat, ptr[2], 18);
  delete (&limmat, ptr[0], 5);

  return 1;
}

/*------------------------------------------------------------------------*/

static int
limmat0 (void)
{
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
limmat1 (void)
{
  Limmat *limmat;
  void *ptr;
  int res;

  limmat = new_Limmat (0);
  ptr = new (limmat, 4711);
  res = (internal_delete_Limmat (limmat) >= 4711);

  return res;
}

/*------------------------------------------------------------------------*/

static int
limmat2 (void)
{
  Limmat *limmat;
  FILE *log;
  int res;

  limmat = new_Limmat (0);
  (void) find (limmat, 1);
  log = fopen ("log/limmat2.log", "w");
  if (log)
    {
      res = !error_Limmat (limmat);
      stats_Limmat (limmat, log);
      fclose (log);
    }
  else
    res = 0;
  delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
stack0 (void)
{
  int res, leaked;
  Limmat *limmat;
  Stack stack;

  limmat = new_Limmat (0);
  init_Stack (limmat, &stack);
  release_Stack (limmat, &stack);
  leaked = internal_delete_Limmat (limmat);
  res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
stack1 (void)
{
  int res, leaked;
  Limmat *limmat;
  Stack stack;

  res = 1;
  limmat = new_Limmat (0);
  init_Stack (limmat, &stack);
  push (limmat, &stack, (void *) 4711);
  if (res)
    res = (4711 == (int) (PTR_SIZED_WORD) pop (&stack));
  if (res)
    res = !stack.internal_count;
  push (limmat, &stack, (void *) (PTR_SIZED_WORD) 13);
  if (res)
    res = (13 == (int) (PTR_SIZED_WORD) pop (&stack));
  if (res)
    res = !stack.internal_count;
  push (limmat, &stack, (void *) (PTR_SIZED_WORD) 17);
  push (limmat, &stack, (void *) (PTR_SIZED_WORD) 19);
  if (res)
    res = (19 == (int) (PTR_SIZED_WORD) pop (&stack));
  if (res)
    res = (17 == (int) (PTR_SIZED_WORD) pop (&stack));
  if (res)
    res = !stack.internal_count;
  release_Stack (limmat, &stack);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_STACK1 10000

static int
stack2 (void)
{
  int res, leaked, i;
  Limmat *limmat;
  Stack stack;

  res = 1;
  limmat = new_Limmat (0);
  init_Stack (limmat, &stack);
  for (i = 0; i < N_STACK1; i++)
    push (limmat, &stack, (void *) (PTR_SIZED_WORD) i);
  for (i = N_STACK1 - 1; res && i >= 0; i--)
    res = (i == (int) (PTR_SIZED_WORD) pop (&stack));
  if (res)
    res = !stack.internal_count;
  release_Stack (limmat, &stack);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_STACK3 11

static int
stack3 (void)
{
  int * a, i, res, leaked;
  Limmat * limmat;
  Stack stack;

  res = 1;
  limmat = new_Limmat (0);
  init_Stack (limmat, &stack);
  for(i = 0; i < N_STACK3; i++)
    push (limmat, &stack, (void*)(PTR_SIZED_WORD)(int)i);

  a = intify_Stack(&stack);

  for(i = 0; res && i < N_STACK3; i++)
    res = (a[i] == i);

  release_Stack (limmat, &stack);
  leaked = internal_delete_Limmat (limmat);
  if(res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
align0 (void)
{
  return sizeof (PTR_SIZED_WORD) == sizeof (void *);
}

/*------------------------------------------------------------------------*/

static int
align1 (void)
{
  int res;

  res = 1;
  if (res)
    res = is_aligned (0, 4);
  if (res)
    res = is_aligned (0, 8);
  if (res)
    res = is_aligned (0, 16);
  if (res)
    res = !is_aligned (1, 4);
  if (res)
    res = !is_aligned (1, 8);
  if (res)
    res = !is_aligned (1, 16);
  if (res)
    res = !is_aligned (3, 4);
  if (res)
    res = !is_aligned (7, 8);
  if (res)
    res = !is_aligned (15, 16);
  if (res)
    res = is_aligned (32, 4);
  if (res)
    res = is_aligned (32, 8);
  if (res)
    res = is_aligned (32, 16);

  return res;
}

/*------------------------------------------------------------------------*/

static int
align2 (void)
{
  int res;

  res = 1;
  if (res)
    res = (align (0, 4) == 0);
  if (res)
    res = (align (0, 8) == 0);
  if (res)
    res = (align (0, 16) == 0);
  if (res)
    res = (align (1, 4) == 4);
  if (res)
    res = (align (1, 8) == 8);
  if (res)
    res = (align (1, 16) == 16);
  if (res)
    res = (align (3, 4) == 4);
  if (res)
    res = (align (7, 8) == 8);
  if (res)
    res = (align (15, 16) == 16);
  if (res)
    res = (align (4, 4) == 4);
  if (res)
    res = (align (8, 8) == 8);
  if (res)
    res = (align (16, 16) == 16);
  if (res)
    res = (align (32, 4) == 32);
  if (res)
    res = (align (32, 8) == 32);
  if (res)
    res = (align (32, 16) == 32);
  if (res)
    res = (align (33, 4) == 36);
  if (res)
    res = (align (33, 8) == 40);
  if (res)
    res = (align (33, 16) == 48);

  return res;
}

/*------------------------------------------------------------------------*/

static int
align3 (void)
{
  int res;

  res = 1;

  if (res)
    res = !(ALL_BUT_SIGN_BITS & SIGN_BIT);

  return res;
}

/*------------------------------------------------------------------------*/

static int
align4 (void)
{
  FILE *log;
  int res;

  log = fopen ("log/align4.log", "w");
  res = 0;
  if (log)
    {
      res = is_aligned_ptr (next_Variable (0), VARIABLE_ALIGNMENT);
      if (res)
	res = (sizeof (Variable) <= (int) next_Variable (0));
      fprintf (log, "sizeof(Variable) = %d\n", (int) sizeof (Variable));
      fprintf (log, "next_Variable(0) = %d\n", (int) next_Variable (0));
      fclose (log);
    }

  return res;
}

/*------------------------------------------------------------------------*/

static int
log0 (void)
{
  int res;

  res = 1;
  if (res)
    res = (msnzb (0) == 0);
  if (res)
    res = (msnzb (1) == 0);
  if (res)
    res = (msnzb (2) == 1);
  if (res)
    res = (msnzb (3) == 1);
  if (res)
    res = (msnzb (4) == 2);
  if (res)
    res = (msnzb (5) == 2);
  if (res)
    res = (msnzb (6) == 2);
  if (res)
    res = (msnzb (7) == 2);
  if (res)
    res = (msnzb (8) == 3);
  if (res)
    res = (msnzb (15) == 3);
  if (res)
    res = (msnzb (16) == 4);
  if (res)
    res = (msnzb (32767) == 14);
  if (res)
    res = (msnzb (32768) == 15);
  if (res)
    res = (msnzb (65535) == 15);
  if (res)
    res = (msnzb (65536) == 16);
  if (res)
    res = (msnzb (1024 * 1024 - 1) == 19);
  if (res)
    res = (msnzb (1024 * 1024) == 20);
  if (res)
    res = (msnzb ((~0)) == sizeof (int) * 8 - 1);

  return res;
}

/*------------------------------------------------------------------------*/

static int
log1 (void)
{
  int res;

  res = 1;
  if (res)
    res = (log2ceil (1) == 0);
  if (res)
    res = (log2ceil (2) == 1);
  if (res)
    res = (log2ceil (3) == 2);
  if (res)
    res = (log2ceil (4) == 2);
  if (res)
    res = (log2ceil (5) == 3);
  if (res)
    res = (log2ceil (6) == 3);
  if (res)
    res = (log2ceil (7) == 3);
  if (res)
    res = (log2ceil (8) == 3);
  if (res)
    res = (log2ceil (9) == 4);
  if (res)
    res = (log2ceil (15) == 4);
  if (res)
    res = (log2ceil (16) == 4);
  if (res)
    res = (log2ceil (17) == 5);
  if (res)
    res = (log2ceil (32767) == 15);
  if (res)
    res = (log2ceil (32768) == 15);
  if (res)
    res = (log2ceil (32769) == 16);
  if (res)
    res = (log2ceil (65535) == 16);
  if (res)
    res = (log2ceil (65536) == 16);
  if (res)
    res = (log2ceil (65537) == 17);
  if (res)
    res = (log2ceil (1024 * 1024 - 1) == 20);
  if (res)
    res = (log2ceil (1024 * 1024) == 20);
  if (res)
    res = (log2ceil (1024 * 1024 + 1) == 21);

  return res;
}

/*------------------------------------------------------------------------*/

static int
arena0 (void)
{
  int res, leaked, alignment;
  Limmat *limmat;
  Arena arena;

  limmat = new_Limmat (0);
  for (alignment = 4; alignment <= 16; alignment *= 2)
    {
      init_Arena (limmat, &arena, alignment);
      release_Arena (limmat, &arena);
    }
  leaked = internal_delete_Limmat (limmat);
  res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_ARENA1 256

static int
arena1 (void)
{
  int res, leaked, alignment;
  int *ptr[N_ARENA1], i, j;
  Limmat *limmat;
  size_t delta;
  Arena arena;

  res = 1;
  limmat = new_Limmat (0);
  for (alignment = 4; res && alignment <= 16; alignment *= 2)
    {
      init_Arena (limmat, &arena, alignment);
      for (i = 0; i < N_ARENA1; i++)
	{
	  ptr[i] = access_Arena (limmat, &arena, i, sizeof (int), &delta);
	  if (delta)
	    for (j = 0; j < i; j++)
	      fix_pointer (ptr + j, delta);
	}

      for (i = 0; res && i < N_ARENA1; i++)
	{
	  res =
	    (ptr[i] ==
	     access_Arena (limmat, &arena, i, sizeof (int), &delta));
	  if (res)
	    res = !delta;
	}

      release_Arena (limmat, &arena);
    }
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
var0 (void)
{
  return not (FALSE) == TRUE;
}

/*------------------------------------------------------------------------*/

static int
var1 (void)
{
  return not (TRUE) == FALSE;
}

/*------------------------------------------------------------------------*/

static int
var2 (void)
{
  return strip (TRUE) == FALSE;
}

/*------------------------------------------------------------------------*/

static int
var3 (void)
{
  Variable v;
  int res;

  v.assignment = &v;
  res = !is_assigned (&v);
  if (res)
    res = (deref (&v) == &v);
  if (res)
    res = (deref (not (&v)) == not (&v));

  return res;
}

/*------------------------------------------------------------------------*/

static int
var4 (void)
{
  Variable v;
  int res;

  v.assignment = FALSE;
  res = is_assigned (&v);
  if (res)
    res = (deref (&v) == FALSE);
  if (res)
    res = (deref (not (&v)) == TRUE);

  return res;
}

/*------------------------------------------------------------------------*/

static int
var5 (void)
{
  Variable v;
  int res;

  v.assignment = TRUE;
  res = is_assigned (&v);
  if (res)
    res = (deref (&v) == TRUE);
  if (res)
    res = (deref (not (&v)) == FALSE);

  return res;
}

/*------------------------------------------------------------------------*/

static int
var6 (void)
{
  int res, leaked;
  Limmat *limmat;
  Variable *v;

  limmat = new_Limmat (0);
  v = find (limmat, 1);
  res = (v->id == 1);
  if (res)
    res = !is_assigned (v);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
var7 (void)
{
  Variable *u, *v, *w;
  int res, leaked;
  Limmat *limmat;

  limmat = new_Limmat (0);
  u = find (limmat, 1);
  v = find (limmat, 1);
  w = find (limmat, -1);
  res = (u == v);
  if (res)
    res = (v == not (w));
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_VAR8 10

static int
var8 (void)
{
  int res, leaked, i, j;
  Variable *u[N_VAR8];
  Limmat *limmat;

  limmat = new_Limmat (0);
  for (i = 0; i < N_VAR8; i++)
    u[i] = find (limmat, i + 1);
  res = 1;
  for (i = 0; res && i < N_VAR8 - 1; i++)
    for (j = i + 1; res && j < N_VAR8; j++)
      res = (u[i] != u[j]);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
var9 (void)
{
  Variable *u, *v;
  int res, leaked;
  Limmat *limmat;

  res = 1;
  limmat = new_Limmat (0);
  v = find (limmat, 4711);
  u = find (limmat, -17);
  if (res)
    res = (var2int (u) == -17);
  if (res)
    res = (var2int (not (u)) == 17);
  if (res)
    res = (var2int (v) == 4711);
  if (res)
    res = (var2int (not (v)) == -4711);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
sort0 (void)
{
  int res, leaked;
  Limmat *limmat;

  limmat = new_Limmat (0);
  init_order (limmat);
  res = is_sorted (limmat);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

#define N_SORT1 3

static int
sort1 (void)
{
  int c[3][2][2], i[3][2], j, k, s, id, res, leaked;
  Limmat *limmat;

  for (s = 0; s < 2; s++)
    for (j = 0; j < 3; j++)
      {
	id = j + 1;
	c[j][s][0] = s ? -id : id;
	c[j][s][1] = 0;
      }

  res = 1;
  for (i[0][0] = 0; i[0][0] < N_SORT1; i[0][0]++)
    for (i[0][1] = 0; i[0][1] < N_SORT1; i[0][1]++)
      for (i[1][0] = 0; i[1][0] < N_SORT1; i[1][0]++)
	for (i[1][1] = 0; i[1][1] < N_SORT1; i[1][1]++)
	  for (i[2][0] = 0; i[2][0] < N_SORT1; i[2][0]++)
	    for (i[2][1] = 0; i[2][1] < N_SORT1; i[2][1]++)
	      {
		limmat = new_Limmat (0);

		for (s = 0; res && s < 2; s++)
		  for (j = 0; res && j < 3; j++)
		    for (k = 0; k < i[j][s]; k++)
		      add_Limmat (limmat, c[j][s]);

		init_order (limmat);
		if (res)
		  res = is_sorted (limmat);
		leaked = internal_delete_Limmat (limmat);
		if (res)
		  res = !leaked;
	      }

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser0 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "/dev/null");
  delete_Parser (parser);
  res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser1 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, 0);
  delete_Parser (parser);
  res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser2 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, 0);
  parse_error (parser, "this is a", " parse error");
  delete_Parser (parser);
  res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser3 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser3.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser4 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser4.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser5 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser5.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser6 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser6.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser7 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser7.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser8 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser8.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser9 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser9.in");
  res = !read_clause (parser);
  if (res)
    res = !parser->error;
  if (res)
    res = !parser->num_specified_max_id;
  if (res)
    res = !parser->num_specified_clauses;
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser10 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser10.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser11 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser11.in");
  res = read_clause (parser);
  if (res)
    res = (parser->num_specified_max_id == 2);
  if (res)
    res = (parser->num_specified_clauses == 2);
  if (res)
    res = (count_Stack (&parser->literals) == 2);
  if (res)
    res = (parser->literals.data.as_array[0] == (void *) 2);
  if (res)
    res = (parser->literals.data.as_array[1] == (void *) -1);
  if (res)
    res = read_clause (parser);
  if (res)
    res = (count_Stack (&parser->literals) == 2);
  if (res)
    res = (parser->literals.data.as_array[0] == (void *) -1);
  if (res)
    res = (parser->literals.data.as_array[1] == (void *) 2);
  if (res)
    res = !read_clause (parser);
  if (res)
    res = !parser->error;
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser12 (void)
{
  Parser *parser;
  Limmat *limmat;
  int res;

  limmat = new_Limmat (0);
  parser = new_Parser (limmat, 0, "log/parser12.in");
  res = !read_clause (parser);
  if (res)
    res = (parser->error != 0);
  delete_Parser (parser);
  if (res)
    res = !internal_delete_Limmat (limmat);

  return res;
}

/*------------------------------------------------------------------------*/

static int
parse_and_print (int n, int expect_parse_error)
{
  char in_name[100], out_name[100], log_name[100];
  int res, leaked, ok;
  Limmat *limmat;
  FILE *log;

  sprintf (in_name, "log/parser%d.in", n);
  sprintf (out_name, "log/parser%d.out", n);
  sprintf (log_name, "log/parser%d.log", n);

  limmat = new_Limmat (0);
  ok = read_Limmat (limmat, 0, in_name);
  if (expect_parse_error)
    res = !ok;
  else
    res = ok;

  log = fopen (log_name, "w");
  if (log)
    {
      print_Limmat (limmat, log);
      fclose (log);
      if (res)
	res = cmp_files (log_name, out_name);
    }
  else
    res = 0;

  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
parser13 (void)
{
  return parse_and_print (13, 0);
}
static int
parser14 (void)
{
  return parse_and_print (14, 0);
}
static int
parser15 (void)
{
  return parse_and_print (15, 0);
}
static int
parser16 (void)
{
  return parse_and_print (16, 1);
}
static int
parser17 (void)
{
  return parse_and_print (17, 1);
}
static int
parser18 (void)
{
  return parse_and_print (18, 0);
}
static int
parser19 (void)
{
  return parse_and_print (19, 1);
}
static int
parser20 (void)
{
  return parse_and_print (20, 0);
}
static int
parser21 (void)
{
  return parse_and_print (21, 1);
}
static int
parser22 (void)
{
  return parse_and_print (22, 0);
}

/*------------------------------------------------------------------------*/

static int
check_sat (const char *prefix, int d, int satisfiable, int max_decisions)
{
  char in_name[100], log_name[100], out_name[100];
  int res, leaked;
  const int *p;
  Limmat *limmat;
  FILE *log;

  limmat = new_Limmat (0);
  sprintf (in_name, "log/%s%d.in", prefix, d);
  sprintf (log_name, "log/%s%d.log", prefix, d);
  sprintf (out_name, "log/%s%d.out", prefix, d);
  res = read_Limmat (limmat, 0, in_name);
  if (res)
    {
      res = (satisfiable == sat (limmat, max_decisions));
      if (satisfiable == 1)
	{
	  p = assignment_Limmat (limmat);
	  if (p && (log = fopen (log_name, "w")))
	    {
	      if (*p)
		{
		  fprintf (log, "%d", *p++);
		  while (*p)
		    fprintf (log, " %d", *p++);
		  fputc ('\n', log);
		}

	      fclose (log);
	      res = cmp_files (out_name, log_name);
	    }
	  else
	    res = 0;
	}
    }
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
sat0 (void)
{
  return check_sat ("sat", 0, 1, -1);
}
static int
sat1 (void)
{
  return check_sat ("sat", 1, 0, -1);
}
static int
sat2 (void)
{
  return check_sat ("sat", 2, 1, -1);
}
static int
sat3 (void)
{
  return check_sat ("sat", 3, 0, -1);
}
static int
sat4 (void)
{
  return check_sat ("sat", 4, 1, -1);
}
static int
sat5 (void)
{
  return check_sat ("sat", 5, 1, -1);
}
static int
sat6 (void)
{
  return check_sat ("sat", 6, 1, -1);
}
static int
sat7 (void)
{
  return check_sat ("sat", 7, 0, -1);
}
static int
sat8 (void)
{
  return check_sat ("sat", 8, 1, -1);
}
static int
sat9 (void)
{
  return check_sat ("sat", 9, 1, -1);
}
static int
sat10 (void)
{
  return check_sat ("sat", 10, 1, -1);
}
static int
sat11 (void)
{
  return check_sat ("sat", 11, 1, -1);
}
static int
sat12 (void)
{
  return check_sat ("sat", 12, 1, -1);
}
static int
sat13 (void)
{
  return check_sat ("sat", 13, 1, -1);
}

/*------------------------------------------------------------------------*/

static int
unit0 (void)
{
  return check_sat ("unit", 0, 0, 0);
}
static int
unit1 (void)
{
  return check_sat ("unit", 1, 0, 0);
}
static int
unit2 (void)
{
  return check_sat ("unit", 2, 1, 0);
}
static int
unit3 (void)
{
  return check_sat ("unit", 3, 1, 0);
}

/*------------------------------------------------------------------------*/

static int
is_prime (int p)
{
  int i;

  for (i = 2; i * i <= p; i++)
    if (!(p % i))
      return 0;

  return 1;
}

/*------------------------------------------------------------------------*/

static int
check_prime (int p)
{
  return check_sat ("prime", p, !is_prime (p), -1);
}

/*------------------------------------------------------------------------*/

static int
prime4 (void)
{
  return check_prime (4);
}
static int
prime9 (void)
{
  return check_prime (9);
}
static int
prime25 (void)
{
  return check_prime (25);
}
static int
prime49 (void)
{
  return check_prime (49);
}
static int
prime121 (void)
{
  return check_prime (121);
}
static int
prime169 (void)
{
  return check_prime (169);
}
static int
prime289 (void)
{
  return check_prime (289);
}
static int
prime361 (void)
{
  return check_prime (361);
}
static int
prime529 (void)
{
  return check_prime (529);
}
static int
prime841 (void)
{
  return check_prime (841);
}
static int
prime961 (void)
{
  return check_prime (961);
}
static int
prime1369 (void)
{
  return check_prime (1369);
}
static int
prime1681 (void)
{
  return check_prime (1681);
}
static int
prime1849 (void)
{
  return check_prime (1849);
}
static int
prime2209 (void)
{
  return check_prime (2209);
}

/*------------------------------------------------------------------------*/

static int
add2 (void)
{
  return check_sat ("add", 2, 0, -1);
}
static int
add4 (void)
{
  return check_sat ("add", 4, 0, -1);
}
static int
add8 (void)
{
  return check_sat ("add", 8, 0, -1);
}
static int
add16 (void)
{
  return check_sat ("add", 16, 0, -1);
}

/*------------------------------------------------------------------------*/

static int
api0 (void)
{
  Limmat *limmat;
  int res, leaked;

  limmat = new_Limmat (0);
  leaked = internal_delete_Limmat (limmat);
  res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
api1 (void)
{
  Limmat *limmat;
  int res, leaked;

  limmat = new_Limmat (0);
  res = (sat_Limmat (limmat, 0) == 1);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
api2 (void)
{
  int res, leaked, clause[1];
  Limmat *limmat;

  limmat = new_Limmat (0);
  clause[0] = 0;
  add_Limmat (limmat, clause);
  res = (sat_Limmat (limmat, 0) == 0);
  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
api3 (void)
{
  int res, leaked, clause[2];
  const int *assignment;
  Limmat *limmat;

  limmat = new_Limmat (0);
  clause[0] = 1;
  clause[1] = 0;
  add_Limmat (limmat, clause);
  res = (sat_Limmat (limmat, 0) == 1);
  if (res)
    {
      assignment = assignment_Limmat (limmat);
      if (res)
	res = (assignment != 0);
      if (res)
	res = (assignment[0] == 1);
      if (res)
	res = (assignment[1] == 0);
    }

  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static int
api4 (void)
{
  int res, leaked, clause[3];
  const int *assignment;
  Limmat *limmat;

  limmat = new_Limmat (0);
  clause[0] = 1;
  clause[1] = 2;
  clause[2] = 0;
  add_Limmat (limmat, clause);
  res = (sat_Limmat (limmat, -1) == 1);
  if (res)
    {
      assignment = assignment_Limmat (limmat);
      if (res)
	res = (assignment != 0);
      if (res)
	res = (assignment[0] && assignment[1] && !assignment[2]);
      if (res)
	res = (assignment[0] != assignment[1]);
      if (res)
	res = (assignment[0] == 1 || assignment[1] == 2);
    }

  leaked = internal_delete_Limmat (limmat);
  if (res)
    res = !leaked;

  return res;
}

/*------------------------------------------------------------------------*/

static void
run (int (*tc) (void), Suite * suite, char *name, int n)
{
  int ok, skip, i, found_pattern;
  char buffer[100];
  double timer;

  sprintf (buffer, "%s%d", name, n);

  for (i = 1, found_pattern = 0; !found_pattern && i < suite->argc; i++)
    found_pattern = (suite->argv[i][0] != '-');

  if (!found_pattern)
    skip = 0;
  else if (isdigit ((int) name[strlen (name) - 1]))
    {
      skip = 1;
      for (i = 1; skip && i < suite->argc; i++)
	skip = strcmp (suite->argv[i], name);
    }
  else
    {
      skip = 1;
      for (i = 1; skip && i < suite->argc; i++)
	skip = (strcmp (suite->argv[i], name)
		&& strcmp (suite->argv[i], buffer));
    }

  if (!skip)
    {
      start_timer (&timer);

      printf ("%-10s ...", buffer);
      fflush (stdout);

      ok = tc ();
      if (ok)
	{
	  suite->ok++;
	  if (!suite->backspaces)
	    printf (" ok     ");
	}
      else
	{
	  suite->failed++;
	  printf (" failed ");
	}

      if (ok && suite->backspaces)
	{
	  for (i = 0; i < 14; i++)
	    fputc ('', stdout);
	  fflush (stdout);
	}
      else
	printf ("in %.2f seconds\n", stop_timer (timer));
    }
}

/*------------------------------------------------------------------------*/
/* Fast test case.
 */
#define TF(name,n) do { run(name ## n, &suite, # name, n); } while(0)

/*------------------------------------------------------------------------*/
/* Slow test case.
 */
#define TS(name,n) \
do { if(!only_fast) run(name ## n, &suite, # name, n); } while(0)

/*------------------------------------------------------------------------*/

static void
line (void)
{
  printf ("-------------------------------------"
	  "------------------------------------\n");
}

/*------------------------------------------------------------------------*/

#define USAGE \
"usage: white [-h|-f|-k] [ <pattern>[<number>] ... ]\n" \
"\n" \
"  -h  print command line option summary\n" \
"  -f  run only fast test cases (usefull if checking is enabled)\n" \
"  -k  print successful test cases (default in dumb terminals like emacs)\n" \
"\n" \
"The list of patterns specifies the test cases to run.  If no pattern is\n" \
"given all test cases are executed.\n"

/*------------------------------------------------------------------------*/

int
main (int argc, char **argv)
{
  int i, only_fast;
  double timer;
  Suite suite;

  start_timer (&timer);

  only_fast = 0;
  suite.backspaces = 0;

  if (isatty (1))
    {
      /* Do not use backspace when running in an EMacs terminal.
       */
      suite.backspaces = !getenv ("TERM") || strcmp (getenv ("TERM"), "dumb");
    }

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-h"))
	{
	  printf (USAGE);
	  exit (0);
	}
      else if (!strcmp (argv[i], "-f"))
	{
	  only_fast = 1;
	}
      else if (!strcmp (argv[i], "-k"))
	{
	  suite.backspaces = 0;
	}
      else if (argv[i][0] == '-')
	{
	  fprintf (stderr,
		   "*** white: unknown command line option '%s' (try '-h')\n",
		   argv[i]);
	  exit (1);
	}
    }

  suite.argc = argc;
  suite.argv = argv;
  suite.ok = 0;
  suite.failed = 0;

  printf ("white box test cases for limmat %s\n", version_Limmat ());
  line ();

  TF (align, 0);
  TF (align, 1);
  TF (align, 2);
  TF (align, 3);
  TF (align, 4);

  TF (log, 0);
  TF (log, 1);

  TF (stack, 0);
  TF (stack, 1);
  TF (stack, 2);
  TF (stack, 3);

  TF (arena, 0);
  TF (arena, 1);

  TF (mem, 0);
  TF (mem, 1);
  TF (mem, 2);

  TF (limmat, 0);
  TF (limmat, 1);
  TF (limmat, 2);

  TF (var, 0);
  TF (var, 1);
  TF (var, 2);
  TF (var, 3);
  TF (var, 4);
  TF (var, 5);
  TF (var, 6);
  TF (var, 7);
  TF (var, 8);
  TF (var, 9);

  TF (sort, 0);
  TF (sort, 1);

  TF (parser, 0);
  TF (parser, 1);
  TF (parser, 2);
  TF (parser, 3);
  TF (parser, 4);
  TF (parser, 5);
  TF (parser, 6);
  TF (parser, 7);
  TF (parser, 8);
  TF (parser, 9);
  TF (parser, 10);
  TF (parser, 11);
  TF (parser, 12);
  TF (parser, 13);
  TF (parser, 14);
  TF (parser, 15);
  TF (parser, 16);
  TF (parser, 17);
  TF (parser, 18);
  TF (parser, 19);
  TF (parser, 20);
  TF (parser, 21);
  TF (parser, 22);

  TF (sat, 0);
  TF (sat, 1);
  TF (sat, 2);
  TF (sat, 3);
  TF (sat, 4);
  TF (sat, 5);
  TF (sat, 6);
  TF (sat, 7);
  TF (sat, 8);
  TF (sat, 9);
  TF (sat, 10);
  TF (sat, 11);
  TF (sat, 12);
  TF (sat, 13);

  TF (unit, 0);
  TF (unit, 1);
  TF (unit, 2);
  TF (unit, 3);

  TF (prime, 4);
  TF (prime, 9);
  TF (prime, 25);

  TS (prime, 49);
  TS (prime, 121);
  TS (prime, 169);
  TS (prime, 289);
  TS (prime, 361);
  TS (prime, 529);
  TS (prime, 841);
  TS (prime, 961);
  TS (prime, 1369);
  TS (prime, 1681);
  TS (prime, 1849);
  TS (prime, 2209);

  TF (add, 2);
  TF (add, 4);
  TS (add, 8);
  TS (add, 16);

  TF (api, 0);
  TF (api, 1);
  TF (api, 2);
  TF (api, 3);
  TF (api, 4);

  if ((!suite.backspaces && suite.ok) || suite.failed)
    line ();

  printf ("%d ok, %d failed, in %.2f seconds\n",
	  suite.ok, suite.failed, stop_timer (timer));

  exit (0);
  return 0;
}

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/
