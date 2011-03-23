import tools
import shlex
import BooleanRewriter

class RsfReader:
    def __init__(self, fd):
        """Read rsf file and store all relations within in database"""
        keys = ["Item", "HasPrompts", "Default", "ItemSelects", "Depends",
                "Choice", "ChoiceItem"]

        self.database = {}
        for key in keys:
            self.database[key] = []

        for line in fd.readlines():
            row = shlex.split(line)
            if len(row) > 1 and row[0] in keys:
                self.database[row[0]].append(row[1:])

    @tools.memoized
    def options(self):
        """Returns all configuration options"""
        # Collect all options
        tristate = {}
        options = set()
        for item in self.database["Item"]:
            if len(item) < 2: continue
            options.add(item[0])
            tristate[item[0]] = item[1].lower() == "tristate"

        # Map them to options
        result = {}
        for option_name in options:
            result[option_name] = Option(self, option_name, tristate[option_name])

        return result


    @tools.memoized
    def collect(self, key, col = 0):
        """Collect all database keys and put them by the n'th column in a dict"""
        result = {}
        for item in self.database[key]:
            if len(item) < col: continue
            result[item[col]] = item[0:col] + item[col+1:]
        return result

    def depends(self):
        return self.collect("Depends")



class OptionInvalid(Exception):
    pass

class OptionNotTristate(Exception):
    pass

class Option (tools.Repr):
    def __init__(self, rsf, name, tristate = False):
        self.rsf = rsf
        self.name = name
        self.tristate = tristate

    def symbol(self):
        if " " in self.name:
            raise OptionInvalid()

        return "CONFIG_%s" % self.name

    def symbol_module(self):
        if " " in self.name:
            raise OptionInvalid()

        if not self.tristate:
            raise OptionNotTristate()
        return "CONFIG_%s_MODULE" % self.name

    def dependency(self, possible_evals = ["y", "m"]):
        depends = self.rsf.depends()
        if not self.name in depends or len(depends[self.name]) == 0:
            return None
        
        depends = depends[self.name][0]

        # Rewrite the dependency
        return str(BooleanRewriter.BooleanRewriter(self.rsf, depends))

    def __unicode__(self):
        return u"<Option %s, tri: %s>" % (self.name, str(self.tristate))
