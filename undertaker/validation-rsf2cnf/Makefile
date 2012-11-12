.PHONY: all clean check

RSF2CNF=../rsf2cnf

all: complete.cnf

clean:
	rm -f complete.cnf

$(RSF2CNF):
	$(MAKE) -C .. rsf2cnf

complete.cnf: mainmodel.cnf files.model $(RSF2CNF)
	$(RSF2CNF) -m files.model -c mainmodel.cnf >complete.cnf

check: complete.cnf
# note: the following tests are fragile, because the cnf variable encodings
# might change in future versions of the program.
	picosat complete.cnf; test $$? -eq 10
#unsatisfiable: FOO=n, FILE_interference.c=y
	picosat complete.cnf -a 4 -a -2; test $$? -eq 20
#satisfiable: FOO=y, FILE_interference.c=y
	picosat complete.cnf -a 4 -a 2; test $$? -eq 10

.PHONY: complete.cnf
