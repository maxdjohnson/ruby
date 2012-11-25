#include "gc_marker.h"
#include <stdlib.h>
#include <stdio.h>
extern void gc_mark_single_threaded(void *objspace);
extern void gc_mark_parallel(void *objspace);

void gc_mark_phase(void *objspace) {
    GC_TEST_LOG("Starting serial", NULL);
    gc_mark_single_threaded(objspace);
    GC_TEST_LOG("Starting parallel", NULL);
    gc_mark_parallel(objspace);

    //Exit immediately to avoid doing anything else
    exit(1);
}
