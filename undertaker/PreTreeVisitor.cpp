#include "PreTreeVisitor.h"

#include "Puma/ErrorStream.h"
#include "Puma/ManipCommander.h"
#include "Puma/PreSemIterator.h"
#include "Puma/PreTreeNodes.h"
#include "Puma/PreTreeToken.h"
#include "Puma/PreSonIterator.h"
#include "Puma/PreTree.h"
#include "Puma/PreParser.h"
#include "Puma/Token.h"
#include "Puma/CUnit.h"
#include <iomanip>
#include <string.h>

using namespace std;

namespace Puma {


// Print the memory address of the current node object and some
// white spaces depending on the tree position.
void PreTreeVisitor::prologue (PreTree* memaddr)
 {
    _depth++;
    *_os << hex << setfill('0') << setw(8)
         << (unsigned long) memaddr << " " << dec;
    for (int i = 0; i < _depth; i++) 
        *_os << "  ";
 }

// Prints all tokens of the node 'node' to stream 'os'
static void print_node (std::ostream &os, PreTree *node)
 {
    Puma::ErrorStream es;
    ManipCommander mc;
    CUnit buf(es);
    mc.copy(&buf, node->startToken(), node->endToken());
    mc.commit();
    buf.print(os);
 }


// Print the number of sons and daughters and the scope of the node.
void PreTreeVisitor::mainPart (PreTree* node)
 {
    Token* token;

    *_os << "\t### " << node->sons () << " " << node->daughters ();
    if ((token = node->startToken ())) {
        const Puma::Location &l = token->location();

        *_os << " from: '"; 
        if (token->type () == TOK_PRE_NEWLINE)
            *_os << "\\n";
        else
            printWithoutNewlines (token->text ());
        *_os << "'";
        *_os << " (" << l.line() << ", " << l.column() << ")";
    }
    if ((token = node->endToken ())) {
        const Puma::Location &l = token->location();
        *_os << " to: '";
        if (token->type () == TOK_PRE_NEWLINE)
            *_os << "\\n";
        else
            printWithoutNewlines (token->text ());
        *_os << "'";
        *_os << " (" << l.line() << ", " << l.column() << ")";
    }
    *_os << endl;

    if (node->daughters ()) {
        // Create the daughter iterator.
        PreTreeIterator* i = new PreSemIterator (node);

        // Go through the daughters.
        for (i->first (); ! i->isDone (); i->next ())
            i->currentItem ()->accept (*this);
    }
}


// Print the token text without newlines.
void PreTreeVisitor::printWithoutNewlines (const char* text) {
  size_t len = strlen(text);
  for (size_t i = 0; i < len; i++) {
    if (text[i] == '\n') {
      *_os << "\\n";
    } else {
      *_os << text[i];
    }
  }
}


// Visit the daughters if exist.
void PreTreeVisitor::epilogue (PreTree* node)
 {
    _depth--;
 }


// Go through the nodes.
void PreTreeVisitor::iterateNodes (PreTree* node)
 {
    PreTreeIterator* i;

    if (_what == SONS)
        i = new PreSonIterator (node);
    else
        i = new PreSemIterator (node);

    for (i->first (); ! i->isDone (); i->next ())
        i->currentItem ()->accept (*this);

    delete i;
 }


void PreTreeVisitor::visitPreTreeToken (PreTreeToken* node)
 {     
    //prologue (node);
    // *_os << "Token '";
    // if (node->token ()->type () == TOK_PRE_NEWLINE)
    //     *_os << "\\n";
    // else
    //     printWithoutNewlines (node->token ()->text ());
    // *_os << "'\t### " << node->sons () << " " << node->daughters () << endl; 
    //_depth--;
 }


void PreTreeVisitor::visitPreError (PreError* node)
 {
    prologue (node);
    *_os << "Error";
    mainPart (node);
    epilogue (node);
 }


void PreTreeVisitor::visitPreCondSemNode (PreCondSemNode* node)
 {
    prologue (node);
    *_os << "=> Value '";
    if (node->value ())
        *_os << "true";
    else
        *_os << "false";
    *_os << "'";
    mainPart (node);
    epilogue (node);
 }

void PreTreeVisitor::visitPreInclSemNode (PreInclSemNode* node)
 {
    prologue (node);
    *_os << "=> File '" << node->unit ()->name () << "'";
    mainPart (node);
    epilogue (node);
 }


void PreTreeVisitor::visitPreProgram_Pre (PreProgram* node)
 {
    prologue (node);
    *_os << "Program";
    mainPart (node);
 }

void PreTreeVisitor::visitPreProgram_Post (PreProgram* node)
 {
    epilogue (node);        
 }


void PreTreeVisitor::visitPreDirectiveGroups_Pre (PreDirectiveGroups* node)
 {
    prologue (node);
    *_os << "DirectiveGroups";
    mainPart (node);
 }

void PreTreeVisitor::visitPreDirectiveGroups_Post (PreDirectiveGroups* node)
 {
    epilogue (node);        
 }


void PreTreeVisitor::visitPreConditionalGroup_Pre (PreConditionalGroup* node)
 {
    prologue (node);
    *_os << "ConditionalGroup";
    mainPart (node);
 }

void PreTreeVisitor::visitPreConditionalGroup_Post (PreConditionalGroup* node)
 {
    *_os << hex << setfill('0') << setw(8)
         << (unsigned long) node << " " << dec;
    for (int i = 0; i < _depth; i++) 
        *_os << "  ";
    *_os << "ConditionalGroupEnd" << std::endl;
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreElsePart_Pre (PreElsePart* node)
 {
    prologue (node);
    *_os << "ElsePart";
    mainPart (node);
 }

void PreTreeVisitor::visitPreElsePart_Post (PreElsePart* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreElifPart_Pre (PreElifPart* node)
 {
    prologue (node);
    *_os << "ElifPart";
    mainPart (node);
 }

void PreTreeVisitor::visitPreElifPart_Post (PreElifPart* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreIfDirective_Pre (PreIfDirective* node)
 {
    prologue (node);
    *_os << "IfDirective";
    mainPart (node);
    print_node(*_os, node);
 }

void PreTreeVisitor::visitPreIfDirective_Post (PreIfDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreIfdefDirective_Pre (PreIfdefDirective* node)
 {
    prologue (node);
    *_os << "IfdefDirective";
    mainPart (node);
    print_node(*_os, node);
 }

void PreTreeVisitor::visitPreIfdefDirective_Post (PreIfdefDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreIfndefDirective_Pre (PreIfndefDirective* node)
 {
    prologue (node);
    *_os << "IfndefDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreIfndefDirective_Post (PreIfndefDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreElifDirective_Pre (PreElifDirective* node)
 {
    prologue (node);
    *_os << "ElifDirective";
    mainPart (node);
    print_node(*_os, node);
 }

void PreTreeVisitor::visitPreElifDirective_Post (PreElifDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreElseDirective_Pre (PreElseDirective* node)
 {
    prologue (node);
    *_os << "ElseDirective";
    mainPart (node);
    print_node(*_os, node);
 }

void PreTreeVisitor::visitPreElseDirective_Post (PreElseDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreEndifDirective_Pre (PreEndifDirective* node)
 {
    prologue (node);
    *_os << "EndifDirective";
    mainPart (node);
    print_node(*_os, node);
 }

void PreTreeVisitor::visitPreEndifDirective_Post (PreEndifDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreIncludeDirective_Pre (PreIncludeDirective* node)
 {
    prologue (node);
    *_os << (node->is_forced () ? "Forced" : "" ) << "IncludeDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreIncludeDirective_Post (PreIncludeDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreAssertDirective_Pre (PreAssertDirective* node)
 {
    prologue (node);
    *_os << "AssertDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreAssertDirective_Post (PreAssertDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreUnassertDirective_Pre (PreUnassertDirective* node)
 {
    prologue (node);
    *_os << "UnassertDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreUnassertDirective_Post (PreUnassertDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreDefineFunctionDirective_Pre (PreDefineFunctionDirective* node)
 {
    prologue (node);
    *_os << "DefineFunctionDir";
    mainPart (node);
 }

void PreTreeVisitor::visitPreDefineFunctionDirective_Post (PreDefineFunctionDirective* node)
 {
    epilogue (node);        
 }
        
void PreTreeVisitor::visitPreDefineConstantDirective_Pre (PreDefineConstantDirective* node)
 {
    prologue (node);
    *_os << "DefineConstantDirective:  ";
    print_node(*_os, node);
    mainPart (node);
 }

void PreTreeVisitor::visitPreDefineConstantDirective_Post (PreDefineConstantDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreUndefDirective_Pre (PreUndefDirective* node)
 {
    prologue (node);
    *_os << "UndefDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreUndefDirective_Post (PreUndefDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreWarningDirective_Pre (PreWarningDirective* node)
 {
    prologue (node);
    *_os << "WarningDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreWarningDirective_Post (PreWarningDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreErrorDirective_Pre (PreErrorDirective* node)
 {
    prologue (node);
    *_os << "ErrorDirective";
    mainPart (node);
 }

void PreTreeVisitor::visitPreErrorDirective_Post (PreErrorDirective* node)
 {
    epilogue (node);        
 }

        
void PreTreeVisitor::visitPreIdentifierList_Pre (PreIdentifierList* node)
 {
    prologue (node);
    *_os << "IdentifierList";
    mainPart (node);
 }

void PreTreeVisitor::visitPreIdentifierList_Post (PreIdentifierList* node)
 {
    epilogue (node);        
 }


void PreTreeVisitor::visitPreTokenList_Pre (PreTokenList* node)
 {
    // prologue (node);
    // *_os << "TokenList";
    // *_os << "\t### " << node->sons () << " " << node->daughters () << endl; 
 }

void PreTreeVisitor::visitPreTokenList_Post (PreTokenList* node)
 {
    // _depth--;        
 }


void PreTreeVisitor::visitPreTokenListPart_Pre (PreTokenListPart* node)
 {
    // prologue (node);
    // *_os << "TokenListPart";
    // *_os << "\t### " << node->sons () << " " << node->daughters () << endl; 
 }

void PreTreeVisitor::visitPreTokenListPart_Post (PreTokenListPart* node)
 {
    // _depth--;
 }


} // namespace Puma
