using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  boolean expression parser
//
///////////////////////////////////////////////////////////////////////////////

struct bool_grammar : public grammar<bool_grammar>
{
    enum {symbolID = 2, not_symbolID, andID, orID, impliesID, iffID,
    };

    template <typename ScannerT>
    struct definition
    {

        rule<ScannerT, parser_context<>, parser_tag<symbolID> > symbol;
        rule<ScannerT, parser_context<>, parser_tag<not_symbolID> > not_symbol;

        rule<ScannerT, parser_context<>, parser_tag<andID> > and_term;
        rule<ScannerT, parser_context<>, parser_tag<orID> > or_term;
        rule<ScannerT, parser_context<>, parser_tag<impliesID> > implies_term;
        rule<ScannerT, parser_context<>, parser_tag<iffID> > iff_term;
        rule<ScannerT> start_rule, group, term, expression;

        definition(bool_grammar const& self)  {
            /*
               Operators (from weak to strong): <->, ->, |, &, !(),
             */
            (void) self;
            symbol       = lexeme_d[ leaf_node_d[ +(alnum_p | ch_p('_')) ]];
            group        = no_node_d[ ch_p('(') ]
                >> start_rule
                >> no_node_d[ ch_p(')') ];
            not_symbol   = no_node_d[ch_p('!')] >> term;

            term = group | not_symbol | symbol;

            /* nodes that aren't leaf nodes */
            and_term     = term         >> *(no_node_d[ch_p("&") >> (*ch_p("&"))] >> term);
            or_term      = and_term     >> *(no_node_d[ch_p("|") >> (*ch_p("|"))] >> and_term);

            implies_term = or_term      >> *(no_node_d[str_p("->")] >> or_term);
            iff_term     = implies_term
                >> *(no_node_d[ str_p("<->") ] >> implies_term);

            start_rule = iff_term
                >> no_node_d[ *(space_p | ch_p("\n") | ch_p("\r"))];
        };

        rule<ScannerT> const& start() const { return start_rule; }
    };
};
