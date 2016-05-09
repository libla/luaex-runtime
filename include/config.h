#if defined(__cplusplus)
#define CEXTERN extern "C"
#else
#define CEXTERN
#endif

#ifdef EXT_DLL
	#if defined(UTIL_CORE)
		#if defined(WIN32)
			#define EXPORTS CEXTERN __declspec(dllexport)
		#else
			#define EXPORTS CEXTERN __attribute__ ((visibility ("default")))
		#endif
	#else
		#if defined(WIN32)
			#define EXPORTS CEXTERN __declspec(dllimport)
		#endif
	#endif
#else
	#define EXPORTS
#endif

#ifndef EXPORTS
	#define EXPORTS CEXTERN
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 i64;
  typedef unsigned __int64 u64;
#else
  typedef long long int i64;
  typedef unsigned long long int u64;
#endif