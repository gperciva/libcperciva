.POSIX:

PROGS=
TESTS=	tests/aws							\
	tests/buildall							\
	tests/buildnothing						\
	tests/buildsingles						\
	tests/crc32							\
	tests/crypto_entropy						\
	tests/elasticarray						\
	tests/events							\
	tests/getopt							\
	tests/heap							\
	tests/humansize							\
	tests/json							\
	tests/monoclock							\
	tests/mpool							\
	tests/parsenum							\
	tests/setuidgid							\
	tests/sha256							\
	tests/valgrind
BINDIR_DEFAULT=	/usr/local/bin
CFLAGS_DEFAULT=	-O2
LIBCPERCIVA_DIR=	.
TEST_CMD=	tests/test_libcperciva.sh

### Shared code between Tarsnap projects.

all:	apisupport-config.h cpusupport-config.h posix-flags.sh
	export CFLAGS="$${CFLAGS:-${CFLAGS_DEFAULT}}";	\
	. ./posix-flags.sh;				\
	. ./cpusupport-config.h;			\
	. ./apisupport-config.h;			\
	export HAVE_BUILD_FLAGS=1;			\
	for D in ${PROGS} ${TESTS}; do			\
		( cd $${D} && ${MAKE} all ) || exit 2;	\
	done

# For "loop-back" building of a subdirectory
buildsubdir: apisupport-config.h cpusupport-config.h posix-flags.sh
	. ./posix-flags.sh;				\
	. ./cpusupport-config.h;			\
	. ./apisupport-config.h;			\
	export HAVE_BUILD_FLAGS=1;			\
	cd ${BUILD_SUBDIR} && ${MAKE} ${BUILD_TARGET}

posix-flags.sh:
	if [ -d ${LIBCPERCIVA_DIR}/POSIX/ ]; then			\
		export CC="${CC}";					\
		cd ${LIBCPERCIVA_DIR}/POSIX;				\
		printf "export \"LDADD_POSIX=";				\
		command -p sh posix-l.sh "$$PATH";			\
		printf "\"\n";						\
		printf "export \"CFLAGS_POSIX=";			\
		command -p sh posix-cflags.sh "$$PATH";			\
		printf "\"\n";						\
	else								\
		:;							\
	fi > $@

apisupport-config.h:
	if [ -d ${LIBCPERCIVA_DIR}/apisupport/ ]; then			\
		export CC="${CC}";					\
		command -p sh						\
		    ${LIBCPERCIVA_DIR}/apisupport/Build/apisupport.sh	\
		    "$$PATH";						\
	else								\
		:;							\
	fi > $@

cpusupport-config.h:
	if [ -d ${LIBCPERCIVA_DIR}/cpusupport/ ]; then			\
		export CC="${CC}";					\
		command -p sh						\
		    ${LIBCPERCIVA_DIR}/cpusupport/Build/cpusupport.sh	\
		    "$$PATH";						\
	else								\
		:;							\
	fi > $@

install:	all
	export BINDIR=$${BINDIR:-${BINDIR_DEFAULT}};	\
	for D in ${PROGS}; do				\
		( cd $${D} && ${MAKE} install ) || exit 2;	\
	done

clean:
	rm -f apisupport-config.h cpusupport-config.h posix-flags.sh
	for D in ${PROGS} ${TESTS}; do				\
		( cd $${D} && ${MAKE} clean ) || exit 2;	\
	done

.PHONY:	test test-clean
test:	all
	${TEST_CMD}

test-clean:
	rm -rf tests-output/ tests-valgrind/

# Developer targets: These only work with BSD make
Makefiles:
	${MAKE} -f Makefile.BSD Makefiles

publish:
	${MAKE} -f Makefile.BSD publish
