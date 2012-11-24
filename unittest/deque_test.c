#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "gc_threading.c"

typedef struct test_case_t {
    VALUE* test_vals;
    VALUE* expected_vals;
    int test_length;
    int max_buf_size;
    deque_t* deque;
} TestCase;
void dump_deque(deque_t* dq);
void init_test_case(TestCase* test, 
                    VALUE* test_vals,
                    VALUE* expected_vals,
                    int test_length,
                    int max_buf_size
                    ) {
    test->test_vals = test_vals;
    test->expected_vals = expected_vals;
    test->test_length = test_length;
    test->max_buf_size = max_buf_size;

    test->deque = malloc(sizeof(deque_t));
    deque_init(test->deque, test->max_buf_size);
}

/* Returns 1 in case of error, 0 otherwise */
int assert_equals(int actual, int expected, const char* msg) {
    if (actual != expected) {
        if (msg)
            printf("%s\n", msg);
        printf("Expected %d, got %d\n", expected, actual);
        return 1;
    }
    else {
        printf("Assertion passed\n");
        return 0;
    }
}

void destroy_test_case(TestCase* test) {
    deque_destroy(test->deque);
}


void do_deque_push_test_call(TestCase* test_case) {
    int i, success;
    for (i = 0; i < test_case->test_length; i++) {
        success = deque_push(test_case->deque, test_case->test_vals[i]);
        assert(success);
    }
}

VALUE do_deque_pop_test_call(TestCase* test_case) {
    return deque_pop(test_case->deque);
}

/* Ensure that the deque has the length specified by the test_case */
int check_deque_length(TestCase* test_case) {
    return assert_equals(test_case->deque->length,
                  test_case->test_length,
                  "Deque_Push failed to properly maintain length\n");
}

/* Ensure that the contents of the deque are those specified by the test_case */
int check_deque_contents(TestCase* test_case) {
    int deque_index, i;
    VALUE expected, actual;
    deque_t* deque = test_case->deque;

    if (assert_equals(deque->length, test_case->test_length, NULL)) {
        return 1;
    }
    for (i = 0; i < test_case->test_length; i++) {      
        deque_index = (deque->head + i) % deque->max_length;
        actual = deque->buffer[deque_index];
        expected = test_case->expected_vals[i];

        if (actual != expected) {
            printf("Expected %lu; got %lu\n", expected, actual);
            printf("Deque is: "); dump_deque(deque); printf("\n");
            return 1;
        }
    }
    return 0;
}

int do_deque_push_test(TestCase* test_case) {
   
    do_deque_push_test_call(test_case);

    return check_deque_length(test_case) ||
        check_deque_contents(test_case);    
}

void dump_deque(deque_t* dq) {
    int i;
    printf("[");
    for(i = 0; i < dq->max_length - 1; i++) {
        printf("%lu, ", dq->buffer[i]);
    }
    printf("%lu", dq->buffer[i]);
    printf("]\n");
}

int test_deque_push_with_wrap() {
    TestCase t;
    TestCase* test = &t;
    VALUE test_vals[3] = {1,2,3};
    printf("test_deque_push_with_wrap:\n");
    init_test_case(test,
                   test_vals,
                   test_vals,                   
                   3,
                   4);
    test->deque->head = 2;
    test->deque->tail = 2;
    return do_deque_push_test(test);
}

int test_pos_mod() {
    int errors = 0;
    printf("test_pos_mod:\n");
    errors += assert_equals(POS_MOD(3, 4), 3, NULL);
    errors += assert_equals(POS_MOD(5, 4), 1, NULL);
    errors += assert_equals(POS_MOD(-1, 4), 3, NULL);
    errors += assert_equals(POS_MOD(-5, 4), 3, NULL);
    return errors != 0;
}

int test_deque_push_maintains_tail_properly() {
    TestCase t;
    TestCase* test = &t;
    VALUE test_vals[3] = {1,2,3};
    printf("test_deque_push_maintains_tail_properly:\n");
    init_test_case(test,
                   test_vals,
                   test_vals,                   
                   3,
                   4);
    do_deque_push_test_call(test);
 
    return assert_equals(test->deque->tail,
                         2,
                         "deque_push failed to maintain tail properly");
}

int test_deque_push_maintains_tail_properly_with_wrap() {
    TestCase t;
    TestCase* test = &t;
    VALUE test_vals[2] = {1,2};
    printf("test_deque_push_maintains_tail_properly_with_wrap:\n");
    init_test_case(test,
                   test_vals,
                   test_vals,                   
                   2,
                   4);
    do_deque_push_test_call(test);
    
    test->deque->head = 2;
    test->deque->tail = 2;
    return assert_equals(test->deque->tail,
                         1,
                         "deque_push failed to maintain tail properly");
}

void set_deque_state_for_pop_test(TestCase* t) {
    int i;
    deque_t* deque = t->deque;
    deque->head = 0;
    deque->tail = t->test_length - 1;
    deque->length = t->test_length;
    for(i = 0; i < t->test_length; i++) {
        deque->buffer[i] = t->test_vals[i];
    }
}

int check_pop_results(TestCase* test) {
    int i, errors = 0;
    for (i = 0; i < test->test_length; i++) {
        errors += assert_equals(do_deque_pop_test_call(test), 
                                test->expected_vals[i],
                                NULL
                                );
    }
    return errors;
}


int test_deque_pop_returns_right_values() {
    TestCase t;
    int i, errors = 0;
    TestCase* test = &t;

    VALUE test_vals[2] = {1,2};
    VALUE expected_vals[2] = {2, 1};
    printf("test_deque_pop_returns_right_values:\n");
    init_test_case(test,
                   test_vals,
                   expected_vals,                   
                   2,
                   4);
    set_deque_state_for_pop_test(test);
    
    return check_pop_results(test) != 0;
}


int test_deque_pop_maintains_tail_with_wrap() {
    TestCase t;
    TestCase* test = &t;
    deque_t* deque;
    int i, errors = 0;

    printf("test_deque_pop_returns_right_values:\n");
    init_test_case(test,
                   NULL,
                   NULL,                   
                   3,
                   3);
    deque = test->deque;
    deque->length = 2;
    deque->head = 2;
    deque->tail = 0;
    deque_pop(deque);
    return assert_equals(test->deque->tail, 2, NULL);
}


int test_wraparound() {  
    return 0;
}


int main (int ARGC, char** ARGV) { 
    int failures = 0;
    int num_tests = 3;
    TestCase tests[num_tests];

    VALUE test_vals[5] = {1,2,3, 4, 5};
    VALUE expected_vals[5] = {1, 2, 3, 4, 5};
    init_test_case(&tests[0],
                   test_vals,
                   expected_vals,
                   5,
                   10);
    failures += test_deque_push_with_wrap();
    failures += test_deque_push_maintains_tail_properly();

    failures += test_deque_pop_returns_right_values();
    failures += test_deque_pop_maintains_tail_with_wrap();
    failures += test_pos_mod();
    printf("Got %d failures\n", failures);

    return failures == 0;
}
