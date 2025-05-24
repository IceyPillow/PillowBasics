#if defined(_WIN64)
#define HAVE_SSE            1
#define HAVE_SSE2           1
#define HAVE_SSE3           1
#define HAVE_SSE4_1         1
#define HAVE_SSE_INTRINSICS 1
#define HAVE_NEON           0
#elif defined(__ANDROID__)
#define HAVE_SSE            0
#define HAVE_SSE2           0
#define HAVE_SSE3           0
#define HAVE_SSE4_1         0
#define HAVE_SSE_INTRINSICS 0
#define HAVE_NEON           1
#endif
