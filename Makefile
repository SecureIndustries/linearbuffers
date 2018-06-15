
subdir-y = \
	src \
	test

test_depends-y = \
	src

include Makefile.lib

install: src test
	install -d ${DESTDIR}/usr/local/bin
	install -m 0755 dist/bin/linearbuffers-compiler ${DESTDIR}/usr/local/bin/linearbuffers-compiler
	
	install -d ${DESTDIR}/usr/local/include/linearbuffers
	install -m 0644 dist/include/linearbuffers/decoder.h ${DESTDIR}/usr/local/include/linearbuffers/decoder.h
	install -m 0644 dist/include/linearbuffers/encoder.h ${DESTDIR}/usr/local/include/linearbuffers/encoder.h
	install -m 0644 dist/include/linearbuffers/jsonify.h ${DESTDIR}/usr/local/include/linearbuffers/jsonify.h
	
	install -d ${DESTDIR}/usr/local/lib
	if [ -f dist/lib/liblinearbuffers-decoder.so ]; then install -m 0755 dist/lib/liblinearbuffers-decoder.so ${DESTDIR}/usr/local/lib/liblinearbuffers-decoder.so; fi
	if [ -f dist/lib/liblinearbuffers-encoder.so ]; then install -m 0755 dist/lib/liblinearbuffers-encoder.so ${DESTDIR}/usr/local/lib/liblinearbuffers-encoder.so; fi
	if [ -f dist/lib/liblinearbuffers-jsonify.so ]; then install -m 0755 dist/lib/liblinearbuffers-jsonify.so ${DESTDIR}/usr/local/lib/liblinearbuffers-jsonify.so; fi

	install -d ${DESTDIR}/usr/local/lib
	if [ -f dist/lib/liblinearbuffers-decoder.a ]; then install -m 0644 dist/lib/liblinearbuffers-decoder.a ${DESTDIR}/usr/local/lib/liblinearbuffers-decoder.a; fi
	if [ -f dist/lib/liblinearbuffers-encoder.a ]; then install -m 0644 dist/lib/liblinearbuffers-encoder.a ${DESTDIR}/usr/local/lib/liblinearbuffers-encoder.a; fi
	if [ -f dist/lib/liblinearbuffers-jsonify.a ]; then install -m 0644 dist/lib/liblinearbuffers-jsonify.a ${DESTDIR}/usr/local/lib/liblinearbuffers-jsonify.a; fi

	install -d ${DESTDIR}/usr/local/lib/pkgconfig
	sed 's?'prefix=/usr/local'?'prefix=${DESTDIR}/usr/local'?g' liblinearbuffers-decoder.pc > ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-decoder.pc
	sed 's?'prefix=/usr/local'?'prefix=${DESTDIR}/usr/local'?g' liblinearbuffers-encoder.pc > ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-encoder.pc
	sed 's?'prefix=/usr/local'?'prefix=${DESTDIR}/usr/local'?g' liblinearbuffers-jsonify.pc > ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-jsonify.pc

uninstall:
	rm -f ${DESTDIR}/usr/local/bin/linearbuffers-compiler
	
	rm -f ${DESTDIR}/usr/local/include/linearbuffers/decoder.h
	rm -f ${DESTDIR}/usr/local/include/linearbuffers/encoder.h
	rm -f ${DESTDIR}/usr/local/include/linearbuffers/jsonify.h
	rm -f ${DESTDIR}/usr/local/include/linearbuffers
	
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-decoder.so
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-encoder.so
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-jsonify.so
	
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-decoder.a
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-encoder.a
	rm -f ${DESTDIR}/usr/local/lib/liblinearbuffers-jsonify.a
	
	rm -f ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-decoder.pc
	rm -f ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-encoder.pc
	rm -f ${DESTDIR}/usr/local/lib/pkgconfig/liblinearbuffers-jsonify.pc
	