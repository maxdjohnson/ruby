#include "ruby/ruby.h"
#include <stdio.h>

extern void gc_mark_defer(void *objspace, VALUE ptr, int lev);
extern void gc_mark_parallel(void* objspace);

void gc_do_mark(void* objspace, VALUE ptr) {

}

void gc_start_mark(void* objspace) {
    int i;
    for (i = 0; i < 200; i++) {
        gc_mark_defer(objspace, i, 1);
    }
}

int main(int argc, char** argv) {
    gc_mark_parallel(NULL);
    return 0;
}

