CFLAGS = -g -I include -I ./ 
BFLAGS = -d

CC = gcc
CFILES = $(shell find ./ -name "*.c")
OBJS = $(CFILES:.c=.o)
LEX = flex
FLEX = flex
BISON = bison

LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")

LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(LFC:.c=.o)
YFO = $(YFC:.c=.o)

parser: syntax $(filter-out $(LFO) $(YFO),$(OBJS))
	$(CC) $(CFLAGS) -o parser $(filter-out $(LFO) $(YFO),$(OBJS))  -lfl -ly

bisonresult:$(YFILE)
	$(BISON) $(BFLAGS) $(YFILE)

scanner:$(LFILE)
	$(LEX) $(LFILE)
	$(CC) $(CFLAGS) scanner lex.yy.c -lfl

syntax: lexical syntax-c
	$(CC) -c $(YFC) $(CFLAGS) -o $(YFO) 

lexical: $(LFILE)
	$(FLEX) -o $(LFC) $(LFILE)
syntax-c: $(YFILE)
	$(BISON) -o $(YFC) -d -v $(YFILE)

run:
	./parser ../Test/test1.cmm test.ir
.PHONY: clean test debug
debug:
	gdb --args parser ../Test/test1.cmm test.ir

clean:
	rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(LFC) $(YFC) $(YFC:.c=.h)
	rm -f *~