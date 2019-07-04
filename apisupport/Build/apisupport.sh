# Should be sourced by `command -p sh path/to/apisupport.sh "$PATH"` from
# within a Makefile.
if ! [ ${PATH} = "$1" ]; then
	echo "WARNING: POSIX violation: $SHELL's command -p resets \$PATH" 1>&2
	PATH=$1
fi
# Standard output should be written to apisupport-config.h, which is both a
# C header file defining APISUPPORT_ARCH_FEATURE macros and sourceable sh
# code which sets CFLAGS_ARCH_FEATURE environment variables.
SRCDIR=`command -p dirname "$0"`

feature() {
	ARCH=$1
	FEATURE=$2
	shift 2;
	if ! [ -f ${SRCDIR}/apisupport-$ARCH-$FEATURE.c ]; then
		return
	fi
	printf "Checking if compiler supports $ARCH $FEATURE feature..." 1>&2
	for CFLAG in "$@"; do
		if ${CC} ${CFLAGS} -D_POSIX_C_SOURCE=200809L ${CFLAG}	\
		    ${SRCDIR}/apisupport-$ARCH-$FEATURE.c 2>/dev/null; then
			rm -f a.out
			break;
		fi
		CFLAG=NOTSUPPORTED;
	done
	case $CFLAG in
	NOTSUPPORTED)
		echo " no" 1>&2
		;;
	"")
		echo " yes" 1>&2
		echo "#define APISUPPORT_${ARCH}_${FEATURE} 1"
		;;
	*)
		echo " yes, via $CFLAG" 1>&2
		echo "#define APISUPPORT_${ARCH}_${FEATURE} 1"
		echo "#ifdef apisupport_dummy"
		echo "export CFLAGS_${ARCH}_${FEATURE}=\"${CFLAG}\""
		echo "#endif"
		;;
	esac
}
