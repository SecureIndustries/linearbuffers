
target-y = \
	linearbuffers-compiler

linearbuffers-compiler_files-y = \
	schema.lex.c \
	schema.lex.h \
	schema.tab.c \
	schema.tab.h \
	main.c

linearbuffers-compiler_schema.lex.c_cflags-y = \
	-Wno-sign-compare \
	-Wno-unused-function
	
linearbuffers-compiler_schema.tab.c_cflags-y = \
	-Wno-implicit-function-declaration

linearbuffers-compiler_ldflags-y = \
	-lfl
	
dist.dir = ../dist
dist.bin-y = \
	linearbuffers-compiler

include ../Makefile.lib

schema.lex.c: schema.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o schema.lex.c --header-file=schema.lex.h $<

schema.lex.h: schema.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o schema.lex.c --header-file=schema.lex.h $<

schema.tab.c: schema.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -Wno-conflicts-rr -d $<

schema.tab.h: schema.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -Wno-conflicts-rr -d $<

clean:
	${Q}${RM} schema.lex.c
	${Q}${RM} schema.lex.h
	${Q}${RM} schema.tab.c
	${Q}${RM} schema.tab.h
