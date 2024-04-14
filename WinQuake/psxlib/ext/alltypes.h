#define _REDIR_TIME64 1
#define _Addr int
#define _Int64 long long
#define _Reg int

#if _MIPSEL || __MIPSEL || __MIPSEL__
#define __BYTE_ORDER 1234
#else
#define __BYTE_ORDER 4321
#endif

#define __LONG_MAX 0x7fffffffL

#ifndef __cplusplus
typedef int wchar_t;
#endif

typedef float float_t;
typedef double double_t;

// typedef struct { long long __ll; long double __ld; } max_align_t;
