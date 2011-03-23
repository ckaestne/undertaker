import tools

class BooleanRewriter(tools.UnicodeMixin):
    def __init__(self, rsf, expr):
        self.rsf = rsf
        self.expr = expr
    def __unicode__(self):
        return "(" + unicode(self.expr) + ")"
