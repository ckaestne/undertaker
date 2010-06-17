#include "KconfigBdd.h"

KconfigBdd::KconfigBdd(std::ifstream &in, bool debug)
    : in_(in),
      items_(in, "Item"),
      depends_(in, "Depends"),
      debug_(debug) {
    m = VariableToBddMap::getInstance();
}

bdd KconfigBdd::kconfigDependencies(std::string variable) {

    bdd X = bddtrue;
    bdd v = m->get_bdd(variable);

    std::string *vp = items_.getValue(variable.substr(sizeof("CONFIG_")-1));
    if (!vp) {
	if (debug_)
	    std::clog <<  "item not found in kconfig" << std::endl;
	return bddtrue;
    } else if (debug_)
	std::clog << "item is of type"  << *vp << std::endl;

    std::string *dependExpr = depends_.getValue(variable.substr(sizeof("CONFIG_")-1));
    if (!dependExpr) {
	if (debug_)
	    std::clog << "item has no dependencies" << std::endl;
	return bddtrue;
    }

    std::clog << " found dependency: "
	      << *vp << " -> " << *dependExpr << std::endl;

    ExpressionParser p(*dependExpr, debug_, true);
    bdd d = p.expression();

    X &= v >> d;

    VariableList list = p.found_variables();
    for (VariableList::iterator i = list.begin();
	 i != list.end(); i++) {
	X &= kconfigDependencies(*i);
    }

    return X;
}
