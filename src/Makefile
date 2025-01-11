
target-y = \
	linearbuffers-compiler

target.o-y = \
	liblinearbuffers-encoder.o

target.a-y = \
	liblinearbuffers-encoder.a

target.so-y = \
	liblinearbuffers-encoder.so

linearbuffers-compiler_files-y = \
	parser.lex.c \
	parser.lex.h \
	parser.tab.c \
	parser.tab.h \
	debug.c \
	schema.c \
	schema-pretty.c \
	schema-c.c \
	schema-js.c \
	main.c

linearbuffers-compiler_parser.lex.c_cflags-y = \
	-Wno-sign-compare \
	-Wno-unused-function
	
linearbuffers-compiler_parser.tab.c_cflags-y = \
	-Wno-implicit-function-declaration

liblinearbuffers-encoder.o_files-y = \
	debug.c \
	encoder.c

liblinearbuffers-encoder.o_cflags-y = \
	-fvisibility=hidden

liblinearbuffers-encoder.a_files-y = \
	debug.c \
	encoder.c

liblinearbuffers-encoder.a_cflags-y = \
	-fvisibility=hidden

liblinearbuffers-encoder.so_files-y = \
	debug.c \
	encoder.c

liblinearbuffers-encoder.so_cflags-y = \
	-fvisibility=hidden

dist.dir = ../dist
dist.base = linearbuffers

dist.bin-y = \
	linearbuffers-compiler

dist.include-y = \
	encoder.h

dist.lib-y = \
	liblinearbuffers-encoder.o \
	liblinearbuffers-encoder.a \
	liblinearbuffers-encoder.so

include ../Makefile.lib

parser.lex.c: parser.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o parser.lex.c --header-file=parser.lex.h $<

parser.lex.h: parser.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o parser.lex.c --header-file=parser.lex.h $<

parser.tab.c: parser.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -d $<

parser.tab.h: parser.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -d $<

clean:
	${Q}${RM} parser.lex.c
	${Q}${RM} parser.lex.h
	${Q}${RM} parser.tab.c
	${Q}${RM} parser.tab.h
