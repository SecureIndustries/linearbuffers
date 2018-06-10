
$(eval tests = $(sort $(subst .lbs,,$(wildcard ??.lbs))))

target-y = \
	${tests}

define test-defaults
	$1_files-y = $1.c
endef

$(eval $(foreach T,${tests},$(eval $(call test-defaults,$T))))
 
include ../Makefile.lib
