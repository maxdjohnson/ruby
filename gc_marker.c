#include "gc_marker.h"



extern void gc_mark_parallel(void *objspace);
/* Entrance point for different marking functions */

void gc_mark_phase(void *objspace) {
    gc_mark_parallel(objspace);    
}
