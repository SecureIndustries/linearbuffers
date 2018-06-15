
target-y = \
	linearbuffers-compiler

target.o-y = \
	liblinearbuffers-encoder.o \
	liblinearbuffers-decoder.o

target.a-y = \
	liblinearbuffers-encoder.a \
	liblinearbuffers-decoder.a

target.so-y = \
	liblinearbuffers-encoder.so \
	liblinearbuffers-decoder.so

linearbuffers-compiler_files-y = \
	parser.lex.c \
	parser.lex.h \
	parser.tab.c \
	parser.tab.h \
	debug.c \
	schema.c \
	main.c

linearbuffers-compiler_parser.lex.c_cflags-y = \
	-fvisibility=hidden \
	-Wno-sign-compare \
	-Wno-unused-function
	
linearbuffers-compiler_parser.tab.c_cflags-y = \
	-fvisibility=hidden \
	-Wno-implicit-function-declaration

linearbuffers-compiler_ldflags-y = \
	-lfl

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

liblinearbuffers-decoder.o_files-y = \
	debug.c \
	decoder.c

liblinearbuffers-decoder.o_cflags-y = \
	-fvisibility=hidden

liblinearbuffers-decoder.a_files-y = \
	debug.c \
	decoder.c

liblinearbuffers-decoder.a_cflags-y = \
	-fvisibility=hidden

liblinearbuffers-decoder.so_files-y = \
	debug.c \
	decoder.c

liblinearbuffers-decoder.so_cflags-y = \
	-fvisibility=hidden

dist.dir = ../dist
dist.base = linearbuffers

dist.bin-y = \
	linearbuffers-compiler
dist.include-y = \
	encoder.h \
	decoder.h
dist.lib-y = \
	liblinearbuffers-encoder.o \
	liblinearbuffers-encoder.a \
	liblinearbuffers-encoder.so \
	liblinearbuffers-decoder.o \
	liblinearbuffers-decoder.a \
	liblinearbuffers-decoder.so

include ../Makefile.lib

parser.lex.c: parser.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o parser.lex.c --header-file=parser.lex.h $<

parser.lex.h: parser.l
	${Q}echo "  FLEX       ${CURDIR}/$@"
	${Q}flex -o parser.lex.c --header-file=parser.lex.h $<

parser.tab.c: parser.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -Wno-conflicts-rr -d $<

parser.tab.h: parser.y
	${Q}echo "  BISON      ${CURDIR}/$@"
	${Q}bison -Wno-conflicts-rr -d $<

clean:
	${Q}${RM} parser.lex.c
	${Q}${RM} parser.lex.h
	${Q}${RM} parser.tab.c
	${Q}${RM} parser.tab.h
