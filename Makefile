CFLAGS=-Wall -g

mcc : main.o gen.o parser.o node.o symbol.o type.o scanner.o misc.o
	$(CC) $(CFLAGS) -o $@ $^

test::
	cd test; make

clean:
	rm -f mcc *.o
	cd test; make clean

main.o : minicc.h
misc.o : minicc.h
scanner.o : minicc.h
type.o : minicc.h
symbol.o : minicc.h
node.o : minicc.h
parser.o : minicc.h
gen.o : minicc.h
