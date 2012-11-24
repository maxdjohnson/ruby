#include "ruby/ruby.h"
#include <pthread.h>
#include <stdlib.h>
/* 
 * Mocks for functions defined in gc.c
 */


/* typedef struct tsafe_st_table_t { */
/*     st_table* table; */
/*     pthread_mutex_t lock; */
/* } tsafe_st_table_t; */


/* tsafe_st_table_t* tsafe_st_table_new () { */
/*     tsafe_st_table_t* table = (tsafe_st_table_t*) malloc(sizeof(tsafe_st_table_t)); */
/*     tsafe_st_table_init(table); */
/*     return table; */
/* } */

/* void tsafe_st_table_init (tsafe_st_table_t* table) { */
    
/* } */


static pthread_mutex_t marked_set_lock;

extern void mocks_init() {
    /* pthread_mutex_init(&marked_set_lock); */
}

//Want: to keep track of everything that gets marked
//Maintain set of marked things

extern void gc_do_mark(void* objspace, VALUE ptr) {
    
}
extern void gc_start_mark(void* objspace) {

}
