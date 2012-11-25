#define TEST_LOG_PREFIX "GCTest"

#ifdef GC_MARK_TEST
#define GC_TEST_LOG(_fmt_str, ...)                              \
    printf(TEST_LOG_PREFIX ": " _fmt_str "\n", __VA_ARGS__)
#else
#define GC_TEST_LOG(_fmt_str, ...)              \
    //noop
#endif

extern void gc_mark_phase(void *objspace);
