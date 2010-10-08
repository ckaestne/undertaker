#include <cstring>
#include <cstdio>
#include <iostream>
#include <malloc.h>
#include "LimBoole.h"

using namespace LimBoole;

/*------------------------------------------------------------------------*/

static unsigned
hash_var (Sat * sat, const char *name)
{
    unsigned res, tmp;
    const char *p;

    res = 0;

    for (p = name; *p; p++)
        {
            tmp = res & 0xf0000000;
            res <<= 4;
            res += *p;
            if (tmp)
                res ^= (tmp >> 28);
        }

    res &= (sat->nodes_size - 1);
    assert (res < sat->nodes_size);

    return res;
}

/*------------------------------------------------------------------------*/

static unsigned
hash_op (Sat * sat, Type type, Node * c0, Node * c1)
{
    unsigned res;

    res = (unsigned) type;
    res += 4017271 * (unsigned) c0;
    res += 70200511 * (unsigned) c1;

    res &= (sat->nodes_size - 1);
    assert (res < sat->nodes_size);

    return res;
}

/*------------------------------------------------------------------------*/

static unsigned
hash (Sat * sat, Type type, Node *c0, Node * c1)
{
    if (type == VAR)
        return hash_var (sat, (char *) c0);
    else
        return hash_op (sat, type, c0, c1);
}

/*------------------------------------------------------------------------*/

static int
eq_var (Node * n, const char *str)
{
    return n->type == VAR && !strcmp (n->data.as_name, str);
}

/*------------------------------------------------------------------------*/

static int
eq_op (Node * n, Type type, Node * c0, Node * c1)
{
    return n->type == type && n->data.as_child[0] == c0
        && n->data.as_child[1] == c1;
}

/*------------------------------------------------------------------------*/

static int
eq (Node * n, Type type, Node *c0, Node * c1)
{
    if (type == VAR)
        return eq_var (n, (char *) c0);
    else
        return eq_op (n, type, c0, c1);
}

/*------------------------------------------------------------------------*/

static Node **
find (Sat * sat, Type type, Node *c0, Node * c1)
{
    Node **p, *n;
    unsigned h;

    h = hash (sat, type, c0, c1);
    for (p = sat->nodes + h; (n = *p); p = &n->next)
        if (eq (n, type, c0, c1))
            break;

    return p;
}

/*------------------------------------------------------------------------*/

static void
enlarge_nodes (Sat * sat)
{
    Node **old_nodes, *p, *next;
    unsigned old_nodes_size, h, i;

    old_nodes = sat->nodes;
    old_nodes_size = sat->nodes_size;
    sat->nodes_size *= 2;
    sat->nodes = (Node **) calloc (sat->nodes_size, sizeof (Node *));

    for (i = 0; i < old_nodes_size; i++)
        {
            for (p = old_nodes[i]; p; p = next)
                {
                    next = p->next;
                    if (p->type == VAR)
                        h = hash_var (sat, p->data.as_name);
                    else
                        h =
                            hash_op (sat, p->type, p->data.as_child[0],
                                     p->data.as_child[1]);
                    p->next = sat->nodes[h];
                    sat->nodes[h] = p;
                }
        }

    free (old_nodes);
}

/*------------------------------------------------------------------------*/

static void
insert (Sat * sat, Node * node)
{
    if (sat->last)
        sat->last->next_inserted = node;
    else
        sat->first = node;
    sat->last = node;
    sat->nodes_count++;
}

/*------------------------------------------------------------------------*/

static Node *
var (Sat * sat, const char *str)
{
    Node **p, *n;

    if (sat->nodes_size <= sat->nodes_count)
        enlarge_nodes (sat);

    p = find (sat, VAR, (Node *) str, 0);
    n = *p;
    if (!n)
        {
            n = (Node *) malloc (sizeof (*n));
            memset (n, 0, sizeof (*n));
            n->type = VAR;
            n->data.as_name = strdup (str);

            *p = n;
            insert (sat, n);
        }

    return n;
}

/*------------------------------------------------------------------------*/

static Node *
op (Sat * sat, Type type, Node * c0, Node * c1)
{
    Node **p, *n;

    if (sat->nodes_size <= sat->nodes_count)
        enlarge_nodes (sat);

    p = find (sat, type, c0, c1);
    n = *p;
    if (!n)
        {
            n = (Node *) malloc (sizeof (*n));
            memset (n, 0, sizeof (*n));
            n->type = type;
            n->data.as_child[0] = c0;
            n->data.as_child[1] = c1;

            *p = n;
            insert (sat, n);
        }

    return n;
}


static void
parse_error (Sat * sat, const char *fmt, ...)
{
    /* FIXME */
}

bool
is_var_letter (int ch)
{
    if (isalnum (ch))
        return true;

    switch (ch) {
    case '-':
    case '_':
    case '.':
    case '[':
    case ']':
    case '$':
    case '@':
        return true;

    default:
        return false;
    }
}

void
enlarge_buffer (Sat * sat)
{
    sat->buffer_size *= 2;
    sat->buffer = (char *) realloc (sat->buffer, sat->buffer_size);
}

static void
unget_char (Sat * sat, int ch)
{
    assert (!sat->saved_char_is_valid);

    sat->saved_char_is_valid = 1;
    sat->saved_char = ch;

    if (ch == '\n')
        {
            sat->x--;
            sat->y = sat->last_y;
        }
    else
        sat->y--;
}

int
next_char (Sat * sat)
{
    int res;

    sat->last_y = sat->y;

    if (sat->saved_char_is_valid) {
        sat->saved_char_is_valid = 0;
        res = sat->saved_char;
    } else {
        res = sat->in_ptr[0];
        sat->in_ptr++;
    }

    if (res == '\n') {
        sat->x++;
        sat->y = 0;
    } else
        sat->y++;

    return res;
}

void
next_token (Sat * sat)
{
    int ch;

    sat->token = ERROR;
    ch = next_char (sat);

 RESTART_NEXT_TOKEN:
    /* Skip all spaces */
    while (isspace ((int) ch))
        ch = next_char (sat);

    /* % is for comment */
    if (ch == '%') {
        while ((ch = next_char (sat)) != '\n' && ch != EOF)
            ;

        goto RESTART_NEXT_TOKEN;
    }

    sat->token_x = sat->x;
    sat->token_y = sat->y;

    if (ch == 0)
        sat->token = DONE;
    else if (ch == '<')
        {
            if (next_char (sat) != '-')
                parse_error (sat, "expected '-' after '<'");
            else if (next_char (sat) != '>')
                parse_error (sat, "expected '>' after '-'");
            else
                sat->token = IFF;
        }
    else if (ch == '-')
        {
            if (next_char (sat) != '>')
                parse_error (sat, "expected '>' after '-'");
            else
                sat->token = IMPLIES;
        }
    else if (ch == '&')
        {
            sat->token = AND;
        }
    else if (ch == '|')
        {
            sat->token = OR;
        }
    else if (ch == '!' || ch == '~')
        {
            sat->token = NOT;
        }
    else if (ch == '(')
        {
            sat->token = LP;
        }
    else if (ch == ')')
        {
            sat->token = RP;
        }
    else if (is_var_letter (ch))
        {
            sat->buffer_count = 0;

            while (is_var_letter (ch))
                {
                    if (sat->buffer_size <= sat->buffer_count + 1)
                        enlarge_buffer (sat);

                    sat->buffer[sat->buffer_count++] = ch;
                    ch = next_char (sat);
                }

            unget_char (sat, ch);
            sat->buffer[sat->buffer_count] = 0;

            if (sat->buffer[sat->buffer_count - 1] == '-')
                parse_error (sat, "variable '%s' ends with '-'", sat->buffer);
            else
                sat->token = VAR;
        }
    else
        parse_error (sat, "invalid character '%c'", ch);
}

static Node *parse_expr (Sat *);

/*------------------------------------------------------------------------*/

static Node *
parse_basic (Sat * sat)
{
    Node *child;
    Node *res;

    res = 0;

    if (sat->token == LP)
        {
            next_token (sat);
            child = parse_expr (sat);
            if (sat->token != RP)
                {
                    if (sat->token != ERROR)
                        parse_error (sat, "expected ')'");
                }
            else
                res = child;
            next_token (sat);
        }
    else if (sat->token == VAR)
        {
            res = var (sat, sat->buffer);
            next_token (sat);
        }
    else if (sat->token != ERROR)
        parse_error (sat, "expected variable or '('");

    return res;
}

/*------------------------------------------------------------------------*/

static Node *
parse_not (Sat * sat)
{
    Node *child, *res;

    if (sat->token == NOT)
        {
            next_token (sat);
            child = parse_not (sat);
            if (child)
                res = op (sat, NOT, child, 0);
            else
                res = 0;
        }
    else
        res = parse_basic (sat);

    return res;
}

/*------------------------------------------------------------------------*/

static Node *
parse_associative_op (Sat * sat, Type type, Node * (*lower) (Sat *))
{
    Node *res, *child;
    int done;

    res = 0;
    done = 0;

    do
        {
            child = lower (sat);
            if (child)
                {
                    res = res ? op (sat, type, res, child) : child;
                    if (sat->token == type)
                        next_token (sat);
                    else
                        done = 1;
                }
            else
                res = 0;
        }
    while (res && !done);

    return res;
}


/*------------------------------------------------------------------------*/

static Node *
parse_and (Sat * sat)
{
    return parse_associative_op (sat, AND, parse_not);
}

/*------------------------------------------------------------------------*/

static Node *
parse_or (Sat * sat)
{
    return parse_associative_op (sat, OR, parse_and);
}

/*------------------------------------------------------------------------*/

static Node *
parse_implies (Sat * sat)
{
    Node *l, *r;

    if (!(l = parse_or (sat)))
        return 0;
    if (sat->token != IMPLIES)
        return l;
    next_token (sat);
    if (!(r = parse_or (sat)))
        return 0;

    return op (sat, IMPLIES, l, r);
}

/*------------------------------------------------------------------------*/

static Node *
parse_iff (Sat * sat)
{
    return parse_associative_op (sat, IFF, parse_implies);
}

/*------------------------------------------------------------------------*/

static Node *
parse_expr (Sat * sat)
{
    return parse_iff (sat);
}



Sat*
LimBoole::parse(const char *expression) {
    Sat *res;

    res = (Sat *) malloc (sizeof (*res));
    memset (res, 0, sizeof (*res));
    res->nodes_size = 2;
    res->nodes = (Node **) calloc (res->nodes_size, sizeof (Node *));
    res->buffer_size = 2;
    res->buffer = (char *) malloc (res->buffer_size);
    res->in_ptr = expression;

    /* Start the parse */
    next_token (res);

    if (res->token == ERROR)
        return res;

    if (!(res->root = parse_expr (res))) {
        res->token = ERROR;
        return res;
    }

    if (res->token == DONE) {
        return res;
    }

    if (res->token != ERROR) {
        parse_error(res, "limboole: expected operator or EOF");
        return res;
    }
    return res;
}

void
LimBoole::release (Sat * sat)
{
    Node *p, *next;

    for (p = sat->first; p; p = next)
        {
            next = p->next_inserted;
            if (p->type == VAR)
                free (p->data.as_name);
            free (p);
        }

    free (sat->idx2node);
    free (sat->nodes);
    free (sat->buffer);
    free (sat);
}


/* The pretty printer */

static void
pp_aux (string &res, Node * node, Type outer)
{
    int le, lt;

    le = outer <= node->type;
    lt = outer < node->type;

    switch (node->type)
        {
        case NOT:
            res.append("!");
            pp_aux (res, node->data.as_child[0], node->type);
            break;
        case IMPLIES:
        case IFF:
            if (le)
                res.append("(");
            pp_aux (res, node->data.as_child[0], node->type);
            if (node->type == IFF) {
                res.append(" <-> ");
            } else {
                res.append(" -> ");
            }
            pp_aux (res, node->data.as_child[1], node->type);
            if (le)
                res.append(")");
            break;

        case OR:
        case AND:
            if (lt)
                res.append("(");
            pp_aux (res, node->data.as_child[0], node->type);
            res.append((node->type == OR ? " | " : " & "));
            pp_aux (res, node->data.as_child[1], node->type);
            if (lt)
                res.append(")");
            break;

        default:
            assert (node->type == VAR);
            res.append(node->data.as_name);
            break;
        }
}

/*------------------------------------------------------------------------*/

static void
pp_and (string &res, Node * node)
{
    if (node->type == AND)
        {
            pp_and (res, node->data.as_child[0]);
            res.append("\n&\n");
            pp_and (res, node->data.as_child[1]);
        }
    else
        pp_aux (res, node, AND);
}

/*------------------------------------------------------------------------*/

static void
pp_or (string &res, Node * node)
{
    if (node->type == OR)
        {
            pp_or (res, node->data.as_child[0]);
            res.append("\n|\n");
            pp_or (res, node->data.as_child[1]);
        }
    else
        pp_aux (res, node, OR);
}

/*------------------------------------------------------------------------*/

static void
pp_and_or (string &res, Node * node, Type outer)
{
    assert (outer > AND);
    assert (outer > OR);

    if (node->type == AND)
        pp_and (res, node);
    else if (node->type == OR)
        pp_or (res, node);
    else
        pp_aux (res, node, outer);
}

/*------------------------------------------------------------------------*/

static void
pp_iff_implies (string &res, Node * node, Type outer)
{
    if (node->type == IFF || node->type == IMPLIES)
        {
            pp_and_or (res, node->data.as_child[0], node->type);
            res.append("\n").append(node->type == IFF ? "<->" : "->").append("\n");
            pp_and_or (res, node->data.as_child[1], node->type);
        }
    else
        pp_and_or (res, node, outer);
}

/*------------------------------------------------------------------------*/

std::string LimBoole::pretty_print (Sat * sat) {
    assert (sat->root);
    string result;
    pp_iff_implies (result, sat->root, DONE);
    result.append("\n");
    return result;
}

/*--------------------- SAT Checker (interface to limmat -----------------*/
void
unit_clause (Sat * sat, int a)
{
    int clause[2];

    clause[0] = a;
    clause[1] = 0;

    add_Limmat (sat->limmat, clause);
}

/*------------------------------------------------------------------------*/

static void
binary_clause (Sat * sat, int a, int b)
{
    int clause[3];

    clause[0] = a;
    clause[1] = b;
    clause[2] = 0;

    add_Limmat (sat->limmat, clause);
}

/*------------------------------------------------------------------------*/

static void
ternary_clause (Sat * sat, int a, int b, int c)
{
    int clause[4];

    clause[0] = a;
    clause[1] = b;
    clause[2] = c;
    clause[3] = 0;

    add_Limmat (sat->limmat, clause);
}

void
tsetin (Sat * sat, bool check_satisfiability)
{
    int num_clauses;
    int sign;
    Node *p;

    num_clauses = 0;

    for (p = sat->first; p; p = p->next_inserted)
        {
            p->idx = ++sat->idx;

            switch (p->type)
                {
                case IFF:
                    num_clauses += 4;
                    break;
                case OR:
                case AND:
                case IMPLIES:
                    num_clauses += 3;
                    break;
                case NOT:
                    num_clauses += 2;
                    break;
                default:
                    assert (p->type == VAR);
                    break;
                }
        }

    sat->idx2node = (Node **) calloc (sat->idx + 1, sizeof (Node *));
    for (p = sat->first; p; p = p->next_inserted)
        sat->idx2node[p->idx] = p;

    for (p = sat->first; p; p = p->next_inserted)
        {
            switch (p->type)
                {
                case IFF:
                    ternary_clause (sat, p->idx,
                                    -p->data.as_child[0]->idx,
                                    -p->data.as_child[1]->idx);
                    ternary_clause (sat, p->idx, p->data.as_child[0]->idx,
                                    p->data.as_child[1]->idx);
                    ternary_clause (sat, -p->idx, -p->data.as_child[0]->idx,
                                    p->data.as_child[1]->idx);
                    ternary_clause (sat, -p->idx, p->data.as_child[0]->idx,
                                    -p->data.as_child[1]->idx);
                    break;
                case IMPLIES:
                    binary_clause (sat, p->idx, p->data.as_child[0]->idx);
                    binary_clause (sat, p->idx, -p->data.as_child[1]->idx);
                    ternary_clause (sat, -p->idx,
                                    -p->data.as_child[0]->idx,
                                    p->data.as_child[1]->idx);
                    break;
                case OR:
                    binary_clause (sat, p->idx, -p->data.as_child[0]->idx);
                    binary_clause (sat, p->idx, -p->data.as_child[1]->idx);
                    ternary_clause (sat, -p->idx,
                                    p->data.as_child[0]->idx, p->data.as_child[1]->idx);
                    break;
                case AND:
                    binary_clause (sat, -p->idx, p->data.as_child[0]->idx);
                    binary_clause (sat, -p->idx, p->data.as_child[1]->idx);
                    ternary_clause (sat, p->idx,
                                    -p->data.as_child[0]->idx,
                                    -p->data.as_child[1]->idx);
                    break;
                case NOT:
                    binary_clause (sat, p->idx, p->data.as_child[0]->idx);
                    binary_clause (sat, -p->idx, -p->data.as_child[0]->idx);
                    break;
                default:
                    assert (p->type == VAR);
                    break;
                }
        }

    assert (sat->root);

    sign = (check_satisfiability) ? 1 : -1;
    unit_clause (sat, sign * sat->root->idx);
}

bool
LimBoole::check(Sat *sat, bool check_satisfiability, int max_decisions) {
    assert (!sat->limmat);
    /* FIXME: Logging? */
    sat->limmat = new_Limmat (0);

    tsetin(sat, check_satisfiability);

    int res = sat_Limmat(sat->limmat, max_decisions);

    delete_Limmat (sat->limmat);
    sat->limmat = 0;

    if (res <= 0) 
        return false;
    return true;
}




