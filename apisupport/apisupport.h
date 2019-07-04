#ifndef _APISUPPORT_H_
#define _APISUPPORT_H_

/*
 * To enable support for non-portable API features at compile time, one or
 * more APISUPPORT_ARCH_FEATURE macros should be defined.  This can be done
 * directly on the compiler command line via -D APISUPPORT_ARCH_FEATURE or
 * -D APISUPPORT_ARCH_FEATURE=1; or a file can be created with the
 * necessary #define lines and then -D APISUPPORT_CONFIG_FILE=apiconfig.h
 * (or similar) can be provided to include that file here.
 */
#ifdef APISUPPORT_CONFIG_FILE
#include APISUPPORT_CONFIG_FILE
#endif

/**
 * The APISUPPORT_FEATURE macro declares the necessary variables and
 * functions for detecting API feature support at run time.  The function
 * defined in the macro acts to cache the result of the ..._detect function
 * using the ..._present and ..._init variables.  The _detect function and the
 * _present and _init variables are turn defined by APISUPPORT_FEATURE_DECL in
 * appropriate apisupport_foo_bar.c file.
 *
 * In order to allow APISUPPORT_FEATURE to be used for features which do not
 * have corresponding APISUPPORT_FEATURE_DECL blocks in another source file,
 * we abuse the C preprocessor: If APISUPPORT_${enabler} is defined to 1, then
 * we access _present_1, _init_1, and _detect_1; but if it is not defined, we
 * access _present_APISUPPORT_${enabler} etc., which we define as static, thus
 * preventing the compiler from emitting a reference to an external symbol.
 *
 * In this way, it becomes possible to issue APISUPPORT_FEATURE invocations
 * for nonexistent features without running afoul of the requirement that
 * "If an identifier declared with external linkage is used... in the entire
 * program there shall be exactly one external definition" (C99 standard, 6.9
 * paragraph 5).  In practice, this means that users of the apisupport code
 * can omit build and runtime detection files without changing the framework
 * code.
 */
#define APISUPPORT_FEATURE__(arch_feature, enabler, enabled)					\
	static int apisupport_ ## arch_feature ## _present ## _APISUPPORT_ ## enabler;		\
	static int apisupport_ ## arch_feature ## _init ## _APISUPPORT_ ## enabler;		\
	static inline int apisupport_ ## arch_feature ## _detect ## _APISUPPORT_ ## enabler(void) { return (0); }	\
	extern int apisupport_ ## arch_feature ## _present_ ## enabled;				\
	extern int apisupport_ ## arch_feature ## _init_ ## enabled;				\
	int apisupport_ ## arch_feature ## _detect_ ## enabled(void);				\
												\
	static inline int									\
	apisupport_ ## arch_feature(void)							\
	{											\
												\
		if (apisupport_ ## arch_feature ## _present_ ## enabled)			\
			return (1);								\
		else if (apisupport_ ## arch_feature ## _init_ ## enabled)			\
			return (0);								\
		apisupport_ ## arch_feature ## _present_ ## enabled = 				\
		    apisupport_ ## arch_feature ## _detect_ ## enabled();			\
		apisupport_ ## arch_feature ## _init_ ## enabled = 1;				\
		return (apisupport_ ## arch_feature ## _present_ ## enabled); 			\
	}											\
	static void (* apisupport_ ## arch_feature ## _dummyptr)(void);				\
	static inline void									\
	apisupport_ ## arch_feature ## _dummyfunc(void)						\
	{											\
												\
		(void)apisupport_ ## arch_feature ## _present ## _APISUPPORT_ ## enabler;	\
		(void)apisupport_ ## arch_feature ## _init ## _APISUPPORT_ ## enabler;		\
		(void)apisupport_ ## arch_feature ## _detect ## _APISUPPORT_ ## enabler;	\
		(void)apisupport_ ## arch_feature ## _present_ ## enabled;			\
		(void)apisupport_ ## arch_feature ## _init_ ## enabled;				\
		(void)apisupport_ ## arch_feature ## _detect_ ## enabled;			\
		(void)apisupport_ ## arch_feature ## _dummyptr;					\
	}											\
	static void (* apisupport_ ## arch_feature ## _dummyptr)(void) = apisupport_ ## arch_feature ## _dummyfunc;	\
	struct apisupport_ ## arch_feature ## _dummy
#define APISUPPORT_FEATURE_(arch_feature, enabler, enabled)	\
	APISUPPORT_FEATURE__(arch_feature, enabler, enabled)
#define APISUPPORT_FEATURE(arch, feature, enabler)				\
	APISUPPORT_FEATURE_(arch ## _ ## feature, enabler, APISUPPORT_ ## enabler)

/*
 * APISUPPORT_FEATURE_DECL(arch, feature):
 * Macro which defines variables and provides a function declaration for
 * detecting the presence of "feature" on the "arch" architecture.  The
 * function body following this macro expansion must return nonzero if the
 * feature is present, or zero if the feature is not present or the detection
 * fails for any reason.
 */
#define APISUPPORT_FEATURE_DECL(arch, feature)				\
	extern int apisupport_ ## arch ## _ ## feature ## _present_1;	\
	extern int apisupport_ ## arch ## _ ## feature ## _init_1;	\
	int apisupport_ ## arch ## _ ## feature ## _present_1 = 0;	\
	int apisupport_ ## arch ## _ ## feature ## _init_1 = 0;		\
	int apisupport_ ## arch ## _ ## feature ## _detect_1(void); \
	int								\
	apisupport_ ## arch ## _ ## feature ## _detect_1(void)

/*
 * List of features.  If a feature here is not enabled by the appropriate
 * APISUPPORT_ARCH_FEATURE macro being defined, it has no effect; but if the
 * relevant macro may be defined (e.g., by Build/apisupport.sh successfully
 * compiling Build/apisupport-ARCH-FEATURE.c) then the C file containing the
 * corresponding run-time detection code (apisupport_arch_feature.c) must be
 * compiled and linked in.
 */

#endif /* !_APISUPPORT_H_ */
