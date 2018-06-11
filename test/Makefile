
$(eval tests = $(sort $(subst .lbs,,$(wildcard ??.lbs))))

target-y = \
	${tests}

all:

define test-defaults
    $1_files-y = \
        $1.c $1-encoder.h
    $1_includes-y = \
        ../dist/include
    $1_libraries-y = \
        ../dist/lib
    $1_ldflags-y = \
    	-Wl,-Bstatic \
        -llinearbuffers-encoder \
		-Wl,-Bdynamic
	$1_depends-y = \
		../dist/lib/liblinearbuffers-encoder.a
    $1-encoder.h: $1.lbs ../dist/bin/linearbuffers-compiler
	${Q}@echo "  GEN        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -e $1-encoder.h
endef

$(eval $(foreach T,${tests},$(eval $(call test-defaults,$T))))

include ../Makefile.lib

clean:
	${Q}${RM} ??-encoder.h
