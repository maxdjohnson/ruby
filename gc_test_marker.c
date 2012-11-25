#include "gc_marker.h"
#include <stdlib.h>
#include <stdio.h>
extern void gc_mark_single_threaded(void *objspace);
extern void gc_mark_parallel(void *objspace);

void gc_mark_phase(void *objspace) {
    printf("Test marker!\n");
    exit(1);
}
