OBJECTS := expr.lex.o expr.tab.o webdna_lex.oo webdna_syntax.oo webdnaAST.oo functions.oo
all: expr webdna ${OBJECTS}
	g++ ${OBJECTS} -lm -o interp

%.o: %.c
	gcc -c $< -o $@ -ggdb3

expr.lex.o:
	gcc -c expr.lex.c -o $@ -ggdb3


expr.tab.o:
	gcc -c expr.tab.c -o $@ -ggdb3

%.oo: %.cpp
	g++ -c $< -o $@ -ggdb3

webdna_syntax.oo:
	g++ -c webdna_syntax.cpp -o $@ -ggdb3

webdna_lex.oo:
	g++ -c webdna_lex.cpp -o $@ -ggdb3

expr:
	flex -o expr.lex.c expr.l
	bison -d --no-lines expr.y

webdna:
	bison -d --no-lines -v -o webdna_syntax.cpp webdna.y
	flex -o webdna_lex.cpp webdna.lex

clean-objs:
		rm -f ${OBJECTS}
		rm -f webdna_lex.cpp webdna_syntax.{c,h}pp webdna.tab.c
		rm -f expr.lex.{c,h} expr.tab.{c,h}
		rm -f *.output

clean: clean-objs
	rm -f interp
