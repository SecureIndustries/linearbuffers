
$(eval tests = $(sort $(subst .lbs,,$(wildcard ??.lbs))))

target-y = \
	${tests}

all:

define test-defaults
    $(eval $1_files-y = $1.c $1-encoder.h)
    $1-encoder.h: $1.lbs ../dist/bin/linearbuffers-compiler
	${Q}@echo "  GEN        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -e $1-encoder.h
endef

$(eval $(foreach T,${tests},$(eval $(call test-defaults,$T))))

include ../Makefile.lib

clean:
	${Q}${RM} ??-encoder.h
