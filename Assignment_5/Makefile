all: rules inputs rules16 inputs16

rules: pktclf.c
	gcc -o $@ $<

rules16: pktclf-16W.c
	gcc -o $@ $<

inputs: pktinfgen.c
	gcc -o $@ $<
