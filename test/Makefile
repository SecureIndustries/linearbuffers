
$(eval tests = $(sort $(subst .lbs,,$(wildcard ??.lbs))))

target-y = \
	${tests}

all:

define test-defaults
    $1_files-y = \
        $1.c \
        $1-encoder.h \
        $1-decoder.h \
        $1-jsonify.h

    $1_includes-y = \
        ../dist/include

    $1_libraries-y = \
        ../dist/lib

    $1_ldflags-y = \
    	-Wl,-Bstatic \
        -llinearbuffers-encoder \
        -llinearbuffers-decoder \
        -llinearbuffers-jsonify \
		-Wl,-Bdynamic

	$1_depends-y = \
		../dist/lib/liblinearbuffers-encoder.a

    $1-encoder.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  GEN        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -e $1-encoder.h

    $1-decoder.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  GEN        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -d $1-decoder.h

    $1-jsonify.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  GEN        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -j $1-jsonify.h
endef

$(eval $(foreach T,${tests},$(eval $(call test-defaults,$T))))

include ../Makefile.lib

clean:
	${Q}${RM} ??-encoder.h
	${Q}${RM} ??-decoder.h
	${Q}${RM} ??-jsonify.h
	${Q}${RM} ??.pretty
