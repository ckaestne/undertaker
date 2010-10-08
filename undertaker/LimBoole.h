#ifndef _limboole_H
#define _limboole_H
#include <string>

namespace LimBoole {
    #include <ctype.h>
    #include <assert.h>

    using std::string;

    /* Include the Limmat library header as C */
    extern "C" {
#include "limmat.h"
    }

    /*------------------------------------------------------------------------*/
    /* These are the node types we support.  They are ordered in decreasing
     * priority: if a parent with type t1 has a child with type t2 and t1 > t2,
     * then pretty printing the parent requires parentheses.  See 'pp_aux' for
     * more details.
     */
    enum Type
    {
        VAR = 0,
        LP = 1,
        RP = 2,
        NOT = 3,
        AND = 4,
        OR = 5,
        IMPLIES = 6,
        IFF = 7,
        DONE = 8,
        ERROR = 9,
    };

    /*------------------------------------------------------------------------*/

    typedef enum Type Type;
    typedef struct Node Node;
    typedef union Data Data;

    /*------------------------------------------------------------------------*/

    union Data
    {
        char *as_name;      /* variable data */
        Node *as_child[2];      /* operator data */
    };

    /*------------------------------------------------------------------------*/

    struct Node
    {
        Type type;
        int idx;            /* tsetin index */
        Node *next;         /* collision chain in hash table */
        Node *next_inserted;        /* chronological list of hash table */
        Data data;
    };

    /*------------------------------------------------------------------------*/

    typedef struct Sat Sat;
    struct Sat
    {
        unsigned nodes_size;
        unsigned nodes_count;
        int idx;
        Node **nodes;
        Node *first;
        Node *last;
        Node *root;
        char *buffer;
        char *name;
        unsigned buffer_size;
        unsigned buffer_count;
        int saved_char;
        int saved_char_is_valid;
        int last_y;
        unsigned x;
        unsigned y;
        Limmat *limmat;
        Type token;
        unsigned token_x;
        unsigned token_y;
        Node **idx2node;
        const char *in_ptr;
        string pp;
    };

    Sat *parse(const char *);
    void release(Sat *);
    string pretty_print(Sat *);
    bool check(Sat *, bool check_satisfiability = true, int max_decisions = -1);

};

#endif
