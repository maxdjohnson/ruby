#include "ruby/ruby.h"
#include <stdio.h>
#include <pthread.h>

#define NTHREADS 4
#define GLOBAL_QUEUE_SIZE 500 /*TODO*/
#define GLOBAL_QUEUE_SIZE_MIN (GLOBAL_QUEUE_SIZE / 4)
#define LOCAL_QUEUE_SIZE 200 /*TODO*/
#define MAX_WORK_TO_GRAB 4
#define MAX_WORK_TO_OFFER 4
#define DEQUE_FULL 0
#define DEQUE_EMPTY -1

extern void gc_do_mark(void* objspace, VALUE ptr);
extern void gc_start_mark(void* objspace);

/**
 * Dequeue
 */

typedef struct deque_struct {
    VALUE* buffer;
    int max_length; //Should be the same size as buffer
    int length;
    int head;
    int tail;
} deque_t;

static void deque_init(deque_t* deque, int max_length) {
    //TODO: check error and handle this reasonably
    VALUE* buffer = (VALUE*) malloc(sizeof(VALUE)*max_length);

    deque->buffer = buffer;
    deque->max_length = max_length;
    deque->length = 0;
    deque->head = deque->tail = -1;
}

static void deque_destroy(deque_t* deque) {
    free(deque->buffer);
}

static void deque_destroy_callback(void* deque) {
    deque_destroy((deque_t*) deque);
}

static int deque_empty_p(deque_t* deque) {
  return deque->length == 0;
}

static int deque_full_p(deque_t* deque) {
  return deque->length == deque->max_length;
}

static int deque_push(deque_t* deque, VALUE val) {
  if (deque_full_p(deque))
    return 0;

  if (deque_empty_p(deque))
    deque->head = 0;

  deque->tail = (deque->tail + 1) % deque->max_length;
  deque->buffer[deque->tail] = val;
  deque->length++;
  return 1;
}

static VALUE deque_pop(deque_t* deque) {
  VALUE rtn;
  if (deque_empty_p(deque))
    return DEQUE_EMPTY;

  rtn = deque->buffer[deque->tail];
  if (deque->length - 1 == 0) {
    //Reset head and tail to beginning
    deque->head = deque->tail = -1;
  }
  else {
    deque->tail = (deque->tail - 1) % deque->max_length;
  }
  deque->length--;
  return rtn;
}

static VALUE deque_pop_back(deque_t* deque) {
  VALUE rtn;

  if (deque_empty_p(deque))
    return DEQUE_EMPTY;

  rtn = deque->buffer[deque->head];
  if (deque->length - 1 == 0) {
    //Reset head and tail to beginning if this call empties the deque
    deque->head = deque->tail = -1;
  }
  else {
    deque->head = (deque->head - 1) % deque->max_length;
  }
  deque->length--;
  return rtn;
}

/**
 * Global Queue
 */

typedef struct global_queue_struct {
    unsigned int waiters;
    unsigned int count;
    deque_t deque;
    pthread_mutex_t lock;
    pthread_cond_t wait_condition;
    unsigned int complete;
} global_queue_t;

static void global_queue_init(global_queue_t* global_queue) {
    global_queue->waiters = 0;
    global_queue->count = 0;
    deque_init(&(global_queue->deque), GLOBAL_QUEUE_SIZE);
    pthread_mutex_init(&global_queue->lock, NULL);
    pthread_cond_init(&global_queue->wait_condition, NULL);
    global_queue->complete = 0;
}

static void global_queue_destroy(global_queue_t* global_queue) {
    deque_destroy(&(global_queue->deque));
    pthread_mutex_destroy(&global_queue->lock);
    pthread_cond_destroy(&global_queue->wait_condition);
}

static void global_queue_pop_work(unsigned long thread_id, global_queue_t* global_queue, deque_t* local_queue) {
    int i;

    printf("Thread %lu aquiring global queue lock\n", thread_id);
    pthread_mutex_lock(&global_queue->lock);
    printf("Thread %lu aquired global queue lock\n", thread_id);
    while (global_queue->count == 0 && !global_queue->complete) {
        global_queue->waiters++;
        printf("Thread %lu checking wait condition. Waiters: %d NTHREADS: %d\n", thread_id, global_queue->waiters, NTHREADS);
        if (global_queue->waiters == NTHREADS) {
            printf("Marking complete + waking threads\n");
            global_queue->complete = 1;
            pthread_cond_broadcast(&global_queue->wait_condition);
        } else {
            // Release the lock and go to sleep until someone signals
            printf("Thread %lu waiting. Waiters: %d\n", thread_id, global_queue->waiters);
            pthread_cond_wait(&global_queue->wait_condition, &global_queue->lock);
            printf("Thread %lu awoken\n", thread_id);
        }
        global_queue->waiters--;
    }

    for (i = 0; i < MAX_WORK_TO_GRAB; i++)  {
        if (deque_empty_p(&global_queue->deque))
            break;
        deque_push(local_queue, deque_pop(&(global_queue->deque)));
    }

    pthread_mutex_unlock(&global_queue->lock);
}

static void global_queue_offer_work(global_queue_t* global_queue, deque_t* local_queue) {
    int i;
    int localqueuesize = local_queue->length;
    if ((global_queue->waiters && localqueuesize > 2) ||
            (global_queue->count < GLOBAL_QUEUE_SIZE_MIN &&
             localqueuesize > LOCAL_QUEUE_SIZE / 2)) {
        if (pthread_mutex_trylock(&global_queue->lock)) {
            //Offer to global
            for (i = 0; i < localqueuesize / 2; i++) {
                deque_push(&(global_queue->deque), deque_pop_back(local_queue));
            }
            if (global_queue->waiters) {
                pthread_cond_broadcast(&global_queue->wait_condition);
            }
            pthread_mutex_unlock(&global_queue->lock);
        }
    }
}

/**
 * Threading code
 */

void* active_objspace;
global_queue_t* global_queue;
pthread_key_t thread_local_deque_k;
static void* mark_run_loop(void* arg) {
    long thread_id = (long) arg;
    printf("Thread %lu started\n", thread_id);
    deque_t deque;
    deque_init(&deque, LOCAL_QUEUE_SIZE);
    pthread_setspecific(thread_local_deque_k, &deque);
    if (thread_id == 0) {
        printf("Thread 0 running start_mark\n");
        gc_start_mark(active_objspace);
    }
    while (1) {
        global_queue_offer_work(global_queue, &deque);
        if (deque_empty_p(&deque)) {
            printf("Thread %lu taking work from the master thread\n", thread_id);
            global_queue_pop_work(thread_id, global_queue, &deque);
        }
        if (global_queue->complete) {
            break;
        }
        VALUE v = deque_pop(&deque);
        printf("Thread %lu marking %lu\n", thread_id, v);
        gc_do_mark(active_objspace, v);
    }
    return NULL;
}

void gc_mark_parallel(void* objspace) {
    global_queue_t queuedata;
    pthread_attr_t attr;
    pthread_t threads[NTHREADS];
    long t;
    void* status;

    active_objspace = objspace;
    global_queue = &queuedata;
    global_queue_init(global_queue);

    pthread_key_create(&thread_local_deque_k, deque_destroy_callback);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (t = 0; t < NTHREADS; t++) {
        pthread_create(&threads[t], &attr, mark_run_loop, (void*)t);
        //TODO: handle error codes
    }
    pthread_attr_destroy(&attr);

    for (t = 0; t < NTHREADS; t++) {
        pthread_join(threads[t], &status);
        //TODO: handle error codes
    }
    global_queue_destroy(global_queue);
}

void gc_mark_defer(void *objspace, VALUE ptr, int lev) {
    deque_t* deque = (deque_t*) pthread_getspecific(thread_local_deque_k);
    if (deque_push(deque, ptr) == 0) {
        global_queue_offer_work(global_queue, deque);
        if (deque_push(deque, ptr) == 0) {
            gc_do_mark(objspace, ptr);
        }
    }
}
