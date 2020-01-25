CFLAGS = -Wall -g -I . -I ..

all: test

test: scanner_test parser_test

test_scanner : test_scanner.o ../scanner.o ../misc.o
	$(CC) $(CFLAGS) -o $@ $^

test_parser : test_parser.o ../parser.o ../node.o ../symbol.o ../type.o \
                ../scanner.o ../misc.o
	$(CC) $(CFLAGS) -o $@ $^

scanner_test : test_scanner
	./test_scanner > test_scanner.output
	-diff -c test_scanner.result test_scanner.output

parser_test : test_parser
	./test_parser test_parser1.c > test_parser1.output
	-diff -c test_parser1.result test_parser1.output > test_parser1.diff
	-cat test_parser1.diff

clean:
	rm -f test_scanner test_parser *.o *.output

test_scanner.o : ../scanner.o ../misc.o ../minicc.h
test_parser.o : ../parser.o ../node.o ../symbol.o ../type.o ../scanner.o \
                ../misc.o ../minicc.h
