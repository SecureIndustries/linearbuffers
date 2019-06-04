
$(eval tests        = $(sort $(subst .c,,$(wildcard ??.c))))
$(eval tests-memcpy = $(addsuffix -memcpy,${tests}))
$(eval tests-js     = $(sort $(wildcard ??.js)))

target-y = \
	${tests} \
	${tests-memcpy}

all:

define test-defaults
    $1_files-y = \
        $1.c \
        $1-encoder.h \
        $1-decoder.h \
        $1-jsonify.h

    $1_includes-y = \
        ../dist/include

    $1_cflags-y = \
    	-D_GNU_SOURCE \
    	-include $1-encoder.h \
    	-include $1-decoder.h \
    	-include $1-jsonify.h 

    $1_libraries-y = \
        ../dist/lib

    $1_ldflags-y = \
        ../dist/lib/liblinearbuffers-encoder.a

    $1_depends-y = \
        $1-encoder.js \
        $1-decoder.js \
        $1-jsonify.js \
        ../dist/lib/liblinearbuffers-encoder.a

    $1-encoder.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-encoder.h -l c -e 1 -i 0

    $1-decoder.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-decoder.h -l c -d 1 -m 0

    $1-jsonify.h: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-jsonify.h -l c -j 1 -m 0

    $1-encoder.js: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-encoder.js -l js -e 1 -i 0

    $1-decoder.js: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-decoder.js -l js -d 1 -m 0

    $1-jsonify.js: $1.lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $1.lbs -o $1-jsonify.js -l js -j 1 -m 0

endef

define test-memcpy-defaults
    $1_files-y = \
        $(subst -memcpy,,$1).c \
        $1-encoder.h \
        $1-decoder.h \
        $1-jsonify.h

    $1_includes-y = \
        ../dist/include

    $1_cflags-y = \
    	-D_GNU_SOURCE \
    	-include $1-encoder.h \
    	-include $1-decoder.h \
    	-include $1-jsonify.h 

    $1_libraries-y = \
        ../dist/lib

    $1_ldflags-y = \
        ../dist/lib/liblinearbuffers-encoder.a

    $1_depends-y = \
        ../dist/lib/liblinearbuffers-encoder.a

    $1-encoder.h: $(subst -memcpy,,$1).lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $(subst -memcpy,,$1).lbs -o $1-encoder.h -l c -e 1 -i 0

    $1-decoder.h: $(subst -memcpy,,$1).lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $(subst -memcpy,,$1).lbs -o $1-decoder.h -l c -d 1 -m 1

    $1-jsonify.h: $(subst -memcpy,,$1).lbs ../dist/bin/linearbuffers-compiler Makefile
	${Q}@echo "  LBS        ${CURDIR}/$$@"
	${Q}@../dist/bin/linearbuffers-compiler -s $(subst -memcpy,,$1).lbs -o $1-jsonify.h -l c -j 1 -m 1
endef

$(eval $(foreach T,${tests},$(eval $(call test-defaults,$T))))
$(eval $(foreach T,${tests-memcpy},$(eval $(call test-memcpy-defaults,$T))))

include ../Makefile.lib

tests: all
	${Q}echo "running tests";
	${Q}for T in ${tests} ${tests-memcpy}; do \
		printf "  $${T} ... "; \
		./$${T} 2>/dev/null 1>/dev/null; \
		if [ $$? != 0 ]; then \
			echo "fail"; \
		else \
			echo "success"; \
		fi \
	done
	${Q}for T in ${tests-js}; do \
		printf "  $${T} ... "; \
		node ./$${T} 2>/dev/null 1>/dev/null; \
		if [ $$? != 0 ]; then \
			echo "fail"; \
		else \
			echo "success"; \
		fi \
	done

clean:
	${Q}${RM} ??-encoder.js
	${Q}${RM} ??-decoder.js
	${Q}${RM} ??-jsonify.js
	${Q}${RM} ??-encoder.h
	${Q}${RM} ??-decoder.h
	${Q}${RM} ??-jsonify.h
	${Q}${RM} ??-memcpy-encoder.h
	${Q}${RM} ??-memcpy-decoder.h
	${Q}${RM} ??-memcpy-jsonify.h
	${Q}${RM} ??.pretty
