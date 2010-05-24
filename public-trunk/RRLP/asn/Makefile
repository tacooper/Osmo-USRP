# This makefile is independant from the autoconf system

# default target will generate erlang modules for parsing RRLP
# and run tests on them

.PHONY: tests clean compile

compile: testall.erl RRLP.erl rrlp.erl parseargsbody.erl util.erl
	erl -s make all -s erlang halt

all: tests

clean:
	rm -f RRLP.erl RRLP.hrl *.beam *.asn1db

# asn -> {erl,hrl}
RRLP.erl: RRLP.set.asn
	erlc -buper_bin +"{inline,optimize,'RRLP'}" RRLP.set.asn

testall.beam: testall.erl
	erlc testall.erl

tests: compile
	./testall.sh

# Your SIM card does not allow a connection to this network
