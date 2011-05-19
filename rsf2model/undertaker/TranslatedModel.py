import sys
import tools
from RsfReader import RsfReader, Choice
import BoolRewriter

class TranslatedModel(tools.UnicodeMixin):
    def __init__(self, rsf):
        self.symbols = []
        self.deps = {}
        self.defaultSelects = {}

        self.always_on = []

        self.rsf = rsf
        for (name, option) in self.rsf.options().items():
            self.translate_option(option)
        for (name, option) in self.rsf.options().items():
            if type(option) == Choice:
                self.translate_choice(option)

        for (item, default_set) in self.rsf.collect("Default", 0, True).items():
            for default in default_set:
                option = self.rsf.options().get(item, None)
                if option:
                    try:
                        self.translate_default(option, default)
                    except BoolRewriter.BoolParserException:
                        # Parsing expression failed, just ignore it
                        pass
        for (item, select_set) in self.rsf.collect("ItemSelects", 0, True).items():
            for select in select_set:
                option = self.rsf.options().get(item, None)
                if option:
                    try:
                        self.translate_select(option, select)
                    except BoolRewriter.BoolParserException:
                        # Parsing of a substring failed, just ignore it
                        pass

    def translate_option(self, option):
        # Generate symbols
        symbol = option.symbol()
        self.symbols.append(symbol)
        self.deps[symbol] = []
        self.defaultSelects[symbol] = []

        if option.omnipresent():
            self.always_on.append(option.symbol())

        if option.tristate():
            symbol_module = option.symbol_module()
            self.symbols.append(symbol_module)
            self.deps[symbol_module] = []

            # mutual exclusion for tristate symbols
            self.deps[symbol].append("!%s" % symbol_module)
            self.deps[symbol_module].append("!%s" % symbol)
            self.deps[symbol_module].append("CONFIG_MODULES")

        # Add dependency
        if option.tristate():
            # For tristate symbols (without _MODULE) the dependency must evaluate to y
            dep = option.dependency(eval_to_module = False)
            if dep:
                self.deps[symbol].insert(0, dep)

            # for _MODULE also "m" is ok
            dep = option.dependency(eval_to_module = True)
            if dep:
                self.deps[option.symbol_module()].insert(0, dep)
        else:
            dep = option.dependency(eval_to_module = True)
            if dep:
                self.deps[symbol].insert(0, dep)

    def translate_choice(self, choice):
        for (symbol, dep) in choice.insert_forward_references().items():
            self.deps[symbol].extend(dep)

    def translate_default(self, option, dependency):
        # dependency: [state, if <expr>]
        if len(dependency) != 2:
            return

        if type(option) == Choice or option.tristate(): # or option.prompts() != 0:
            return

        [state, cond] = dependency
        if state == "y" and cond == "y" \
               and option.prompts() == 0 \
               and not option.tristate():
            self.always_on.append(option.symbol())
            # we add a free item to the defaultSelect list. this is
            # important in the following scenario:
            # 1. Item ist default on
            # 2. item is selected by other item
            # 3. without the free item it is implied, that the second
            #    item is on, byt with default to y this isn't right
            #
            # Difference:
            # A DEPS && (SELECT1)
            # A DEPS && (__FREE__ || SELECT1)
            self.defaultSelects[option.symbol()].append(tools.new_free_item())
        elif state == "y" or cond == "y":
            expr = state
            if state == "y":
                expr = cond
            expr =  str(BoolRewriter.BoolRewriter(self.rsf, expr, eval_to_module = True).rewrite())
            self.defaultSelects[option.symbol()].append(expr)
        # Default FOO "BAR" "BAZ"
        elif len(state) > 1 and len(cond) > 1:
            expr =  str(BoolRewriter.BoolRewriter(self.rsf, "(%s) && (%s)" % (state, cond),
                                                  eval_to_module = True).rewrite())
            self.defaultSelects[option.symbol()].append(expr)


    def translate_select(self, option, select):
        if type(option) == Choice:
            return

        if len(select) != 2:
            return

        selected = self.rsf.options().get(select[0], None)
        if not selected:
            return

        # We only look on boolean selected options
        if type(selected) == Choice or selected.tristate():
            return

        expr = select[1]
        if expr == "y":
            # select foo if y
            self.defaultSelects[selected.symbol()].append(option.symbol())
            self.deps[option.symbol()].append(selected.symbol())
            if option.tristate():
                self.defaultSelects[selected.symbol()].append(option.symbol_module())
                self.deps[option.symbol_module()].append(selected.symbol())
        else:
            # select foo if expr
            expr = str(BoolRewriter.BoolRewriter(self.rsf, expr, eval_to_module = True).rewrite())
            if expr == "":
                # We are in a Choice, and the choice is the only dependency....
                imply = selected.symbol()
            else:
                imply = "((%s) -> %s)" %(expr, selected.symbol())
            self.defaultSelects[selected.symbol()].append(option.symbol())
            self.deps[option.symbol()].append(imply)
            if option.tristate():
                self.defaultSelects[selected.symbol()].append(option.symbol_module())
                self.deps[option.symbol_module()].append(imply)


    def __unicode__(self):
        result = u""
        result += u"I: Items-Count: %d\n" % len(self.symbols)
        result += u"I: Format: <variable> [presence condition]\n"
        if len(self.always_on) > 0:
            result += "UNDERTAKER_SET ALWAYS_ON " + (" ".join(map(lambda x: '"' + x + '"', self.always_on))) + "\n"

        for symbol in self.symbols:
            expression = ""
            deps = self.deps.get(symbol, [])
            if symbol in self.defaultSelects and len(self.defaultSelects[symbol]) > 0:
                dS = self.defaultSelects[symbol]
                deps.append("(" + (" || ".join(dS)) + ")")

            expression = " && ".join(deps)
            if expression == "":
                result += "%s\n" % symbol
            else:
                result += "%s \"%s\"\n" % (symbol, expression)
        return result

