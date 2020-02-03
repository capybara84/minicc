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
	-diff test_scanner.result test_scanner.output

parser_test : test_parser
	./test_parser test_parser1.c > test_parser1.output
	-diff test_parser1.result test_parser1.output > test_parser1.diff
	-cat test_parser1.diff
	./test_parser test_parser2.c > test_parser2.output
	-diff test_parser2.result test_parser2.output > test_parser2.diff
	-cat test_parser2.diff
	./test_parser test_parser3.c > test_parser3.output
	-diff test_parser3.result test_parser3.output > test_parser3.diff
	-cat test_parser3.diff
	./test_parser test_parser4.c > test_parser4.output
	-diff test_parser4.result test_parser4.output > test_parser4.diff
	-cat test_parser4.diff
	./test_parser test_parser5.c > test_parser5.output
	-diff test_parser5.result test_parser5.output > test_parser5.diff
	-cat test_parser5.diff

clean:
	rm -f test_scanner test_parser *.o *.output

test_scanner.o : ../scanner.o ../misc.o ../minicc.h
test_parser.o : ../parser.o ../node.o ../symbol.o ../type.o ../scanner.o \
                ../misc.o ../minicc.h
